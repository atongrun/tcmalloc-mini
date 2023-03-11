#include "PageCache.h"

PageCache PageCache::_sInst;

// page cache ����һ��span
Span* PageCache::NewSpan(size_t k)
{
	// �ȶ���һ��
	assert(k > 0);

	// ���k > 128ҳ ��ϵͳ����
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

	// �ȼ���k��Ͱ����û��Span
	if (!_spanLists[k].Empty())
	{
		// ����ע���Ѿ����������⣡����
		Span* kSpan = _spanLists[k].PopFront();
		for (PAGE_ID i = 0; i < kSpan->_n; ++i)
		{
			//_idSpanMap[kSpan->_pageId + i] = kSpan;
			_idSpanMap.set(kSpan->_pageId + i, kSpan);
		}
		// ͷɾ��һ����������ȥʵ��PopFront���߼�
		return kSpan;
	}

	//���k��ͰΪ�գ��������Ͱ���ҵ�һ���ǿ�Ͱ��ֱ�����һ��Ͱ
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanLists[i].Empty())
		{
			Span* nSpan = _spanLists[i].PopFront();
			// ��
			//Span* kSpan = new Span;
			Span* kSpan = _spanPool.New();

			// �ֳ�npan ��һ�� kҳ���أ�Ȼ��n-kҳ��span�ҵ�n-k��Ͱ��
			// ����ע�⣬ֻ��Ҫ��ҳ�ź�ҳ���Ϳ����ˣ�span�ҵ��ڴ�ĵ�ַ��ҳ��ȡ
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_n -= k;
			nSpan->_pageId += k;
			

			_spanLists[nSpan->_n].PushFront(nSpan);
			// �洢nSpan����βҳ��nSpan����ӳ�䣬����page cache�����ڴ�

			/*_idSpanMap[nSpan->_pageId] = nSpan;
			_idSpanMap[nSpan->_pageId + nSpan->_n - 1] = nSpan;*/

			_idSpanMap.set(nSpan->_pageId, nSpan);
			_idSpanMap.set(nSpan->_pageId + nSpan->_n - 1, nSpan);


			// ���ص�kSpan��ҲҪ����id��span��ӳ�䣬����central cache����С���ڴ�ʱ�����Ҷ�Ӧ��span
			
			for (PAGE_ID i = 0; i < kSpan->_n; ++i)
			{
				//_idSpanMap[kSpan->_pageId + i] = kSpan;
				_idSpanMap.set(kSpan->_pageId + i, kSpan);

			}
			return kSpan;
		}
	}

	// ������˵�����������λ�ö�û�д�ҳ��span
	// ��ʱ�����Ҫ�������һ��128ҳ��span
	//Span* bigSpan = new Span;
	Span* bigSpan = _spanPool.New();

	void* ptr = SystemAlloc(NPAGES-1);


	bigSpan->_n = NPAGES - 1;
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;

	_spanLists[bigSpan->_n].PushFront(bigSpan);

	return NewSpan(k);
}

// Easter egg  ����һ��������Ϸ �� �� �� �� �� �� ��������ƽ��������벻Ҫ������
/**
 &#97;&#72;&#82;&#48;&#99;&#68;&#111;&#118;&#76;&#122;&#69;&#121;&#77;&#83;&#52;&#49;&#76;&#106;&#69;&#49;&#77;&#67;&#52;&#53;&#78;&#106;&#111;&#52;&#77;&#68;&#103;&#119;&#76;&#50;&#108;&#117;&#90;&#71;&#86;&#52;&#76;&#109;&#104;&#48;&#98;&#87;&#119;&#61;
**/

// ��ȡ����span��ӳ��
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

// �ͷſ���span�ص�PageCache�����ϲ����ڵ�span
void PageCache::ReleaseSpanToPageCache(Span* span)
{
	if (span->_n > NPAGES - 1)
	{
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		_spanPool.Delete(span);
		return;
	}
	// �ϲ����ڵ�span����ǰ�ϲ��������⣺ǰ���span������þͲ��ܽ��кϲ�
	while (1)
	{
		PAGE_ID prevId = span->_pageId - 1;
		
		//auto ret = _idSpanMap.find(prevId);
		////ǰ�ߵ�ҳ��û���˾Ͳ��Ͳ���
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}

		auto ret = (Span*)_idSpanMap.get(prevId);
		if (ret == nullptr)
		{
			break;
		}

		// ǰ������ҳ��ʹ�ã����ϲ�
		//Span* prevSpan = ret->second;
		Span* prevSpan = ret;

		if (prevSpan->_isUse == true)
		{
			break;
		}

		//����128ҳ��spanҲ���ϲ�
		if (prevSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		// ����ϲ����߼�

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

		//����128ҳ��spanҲ���ϲ�
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