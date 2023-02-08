#include "CentralCache.h"

CentralCache CentralCache::_sInst; //类外实现，不用加static


// 获取一个非空的span
Span* CentralCache::GetOneSpan(SpanList& list, size_t byte_size)
{
	// ...
	return nullptr;
}

// 从中心缓存获取一定数量的对象给thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	// 先算size大小的对象，对应哪个桶，然后就去哪个桶去申请
	size_t index = SizeClass::Index(size);
	// 申请-加桶锁
	_spanLists[index]._mtx.lock();

	// 首先保证有一个非空的Span
	Span* span = GetOneSpan(_spanLists[index], size);
	assert(span);
	assert(span->_freeList);

	// 从span中获取batchNum个对象
	// 如果不够batchNum，有几个拿几个，返回实际拿到的个数

	start = span->_freeList;
	end = start;
	int actualNum = 1;
	size_t i = 0;
	while (i<batchNum && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		++i;
		++actualNum;
	}

	// 拿走 actualNum个对象
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;
	
	// 解锁
	_spanLists[index]._mtx.unlock();

	return actualNum;
}