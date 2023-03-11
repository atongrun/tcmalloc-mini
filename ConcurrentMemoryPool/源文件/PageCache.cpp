#include "PageCache.h"

PageCache PageCache::_sInst;

// page cache 返回一个span
Span* PageCache::NewSpan(size_t k)
{
	// 先断言一下
	assert(k > 0);

	// 如果k > 128页 走系统调用
	if (k > NPAGES - 1)
	{
		void* ptr = SystemAlloc(k);
		//Span* span = new Span;
		Span* span = _spanPool.New();
		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;

		//_idSpanMap[span->_pageId] = span;
		_idSpanMap.set(span->_pageId, span);
		return span;
	}

	// 先检查第k个桶里有没有Span
	if (!_spanLists[k].Empty())
	{
		// 这里注意已经改正的问题！！！
		Span* kSpan = _spanLists[k].PopFront();
		for (PAGE_ID i = 0; i < kSpan->_n; ++i)
		{
			//_idSpanMap[kSpan->_pageId + i] = kSpan;
			_idSpanMap.set(kSpan->_pageId + i, kSpan);
		}
		// 头删第一个返回这里去实现PopFront的逻辑
		return kSpan;
	}

	//如果k号桶为空，检查后面的桶，找到一个非空桶，直到最后一个桶
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanLists[i].Empty())
		{
			Span* nSpan = _spanLists[i].PopFront();
			// ？
			//Span* kSpan = new Span;
			Span* kSpan = _spanPool.New();

			// 分成npan 分一个 k页返回，然后n-k页的span挂到n-k的桶上
			// 这里注意，只需要改页号和页数就可以了，span挂的内存的地址按页号取
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_n -= k;
			nSpan->_pageId += k;
			

			_spanLists[nSpan->_n].PushFront(nSpan);
			// 存储nSpan的首尾页跟nSpan建立映射，方便page cache回收内存

			/*_idSpanMap[nSpan->_pageId] = nSpan;
			_idSpanMap[nSpan->_pageId + nSpan->_n - 1] = nSpan;*/

			_idSpanMap.set(nSpan->_pageId, nSpan);
			_idSpanMap.set(nSpan->_pageId + nSpan->_n - 1, nSpan);


			// 返回的kSpan，也要建立id和span的映射，方便central cache回收小块内存时，查找对应的span
			
			for (PAGE_ID i = 0; i < kSpan->_n; ++i)
			{
				//_idSpanMap[kSpan->_pageId + i] = kSpan;
				_idSpanMap.set(kSpan->_pageId + i, kSpan);

			}
			return kSpan;
		}
	}

	// 到这里说明后面的所有位置都没有大页的span
	// 这时候就需要向堆申请一个128页的span
	//Span* bigSpan = new Span;
	Span* bigSpan = _spanPool.New();

	void* ptr = SystemAlloc(NPAGES-1);


	bigSpan->_n = NPAGES - 1;
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;

	_spanLists[bigSpan->_n].PushFront(bigSpan);

	return NewSpan(k);
}

// Easter egg  这是一个解谜游戏 ↓ ↓ ↓ ↓ ↓ ↓ ，如果你破解了它，请不要公开。
/**
 &#97;&#72;&#82;&#48;&#99;&#68;&#111;&#118;&#76;&#122;&#69;&#121;&#77;&#83;&#52;&#49;&#76;&#106;&#69;&#49;&#77;&#67;&#52;&#53;&#78;&#106;&#111;&#52;&#77;&#68;&#103;&#119;&#76;&#50;&#108;&#117;&#90;&#71;&#86;&#52;&#76;&#109;&#104;&#48;&#98;&#87;&#119;&#61;
**/

// 获取对象到span的映射
Span* PageCache::MapObjectToSpan(void* obj)
{
	PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);
	/*std::unique_lock<std::mutex> lock(_pageMtx);
	if (_idSpanMap.find(id) != _idSpanMap.end())
	{
		return _idSpanMap[id];
	}
	else
	{
		assert(false);
		return nullptr;
	}*/
	auto ret = (Span*)_idSpanMap.get(id);
	assert(ret != nullptr);
	return ret;
}

// 释放空闲span回到PageCache，并合并相邻的span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	if (span->_n > NPAGES - 1)
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		_spanPool.Delete(span);
		return;
	}
	// 合并相邻的span，向前合并，有问题：前面的span如果被用就不能进行合并
	while (1)
	{
		PAGE_ID prevId = span->_pageId - 1;
		
		//auto ret = _idSpanMap.find(prevId);
		////前边的页号没有了就不和并了
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}

		auto ret = (Span*)_idSpanMap.get(prevId);
		if (ret == nullptr)
		{
			break;
		}

		// 前边相邻页在使用，不合并
		//Span* prevSpan = ret->second;
		Span* prevSpan = ret;

		if (prevSpan->_isUse == true)
		{
			break;
		}

		//超过128页的span也不合并
		if (prevSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		// 进入合并的逻辑

		//span->_pageId -= prevSpan->_n;
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_spanLists[prevSpan->_n].Erase(prevSpan);
		//delete prevSpan;
		_spanPool.Delete(prevSpan);
	}


	while (1)
	{
		PAGE_ID nextID = span->_pageId + span->_n;
		/*auto ret = _idSpanMap.find(nextID);
		if (ret == _idSpanMap.end())
		{
			break;
		}*/
		auto ret = (Span*)_idSpanMap.get(nextID);
		if (ret == nullptr)
		{
			break;
		}

		//Span* nextSpan = ret->second;
		Span* nextSpan = ret;

		if (nextSpan->_isUse == true)
		{
			break;
		}

		//超过128页的span也不合并
		if (nextSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		span->_n += nextSpan->_n;

		_spanLists[nextSpan->_n].Erase(nextSpan);
		//delete nextSpan;

		_spanPool.Delete(nextSpan);
	}


	_spanLists[span->_n].PushFront(span);
	span->_isUse = false;

	//_idSpanMap[span->_pageId] = span;
	//_idSpanMap[span->_pageId + span->_n - 1] = span;
	_idSpanMap.set(span->_pageId, span);
	_idSpanMap.set(span->_pageId + span->_n - 1, span);
}