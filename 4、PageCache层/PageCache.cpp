#include "PageCache.h"

PageCache PageCache::_sInst;

// page cache 返回一个span
Span* PageCache::NewSpan(size_t k)
{
	// 先断言一下
	assert(k > 0 && k < NPAGES);

	// 先检查第k个桶里有没有Span
	if (!_spanLists[k].Empty())
	{
		// 头删第一个返回这里去实现PopFront的逻辑
		return _spanLists[k].PopFront();
	}

	//如果k号桶为空，检查后面的桶，找到一个非空桶，直到最后一个桶
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanLists[i].Empty())
		{
			Span* nSpan = _spanLists[i].PopFront();
			// ？
			Span* kSpan = new Span;

			// 分成npan 分一个 k页返回，然后n-k页的span挂到n-k的桶上
			// 这里注意，只需要改页号和页数就可以了，span挂的内存的地址按页号取
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_n -= k;
			nSpan->_pageId += k;


			_spanLists[nSpan->_n].PushFront(nSpan);
			return kSpan;
		}
	}

	// 到这里说明后面的所有位置都没有大页的span
	// 这时候就需要向堆申请一个128页的span
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(NPAGES-1);


	bigSpan->_n = NPAGES - 1;
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHITF;

	_spanLists[bigSpan->_n].PushFront(bigSpan);

	return NewSpan(k);
}
