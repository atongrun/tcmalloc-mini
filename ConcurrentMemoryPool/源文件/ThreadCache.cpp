#include "ThreadCache.h"
#include "CentralCache.h"

//从中心缓存获取对象
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	// 慢开始反馈调节算法
	// 先算要多少个size大小的对象
	// 这里注意，Windows.h中有个宏min，如果用std会冲突报错
	size_t batchNum = min(SizeClass::NumMoveSize(size), _freeLists[index].MaxSize());

	if (_freeLists[index].MaxSize() == batchNum)
	{
		_freeLists[index].MaxSize() += 1;
	}


	// 知道要多少个对象后就向central cache层要batchNum个对象
	void* start;
	void* end;

	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
	assert(actualNum >= 1);

	if (actualNum == 1)
	{
		//只有一个
		assert(start == end);
		return start;
	}
	else
	{
		// 将NextObj(start)-end 头插进自由链表，返回start, 实际push了actualNum-1块内存
		_freeLists[index].PushRange(NextObj(start), end, actualNum-1);
		return start;
	}

}


// 申请内存
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES); 
	// 先算size个字节映射到哪个桶，但是不同的字节范围，映射的规则不同，用一个类管理

	//找到当前字节数的对齐数，以及对应哈希桶的下标
	size_t alignSize = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);

	// 如果所在自由链表不为空，头删返回头节点
	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		//为空，则去CentralCache层获取
		return FetchFromCentralCache(index, alignSize);
	}

}
// 释放内存
void ThreadCache::Deallocate(void* obj, size_t size)
{
	assert(obj);
	assert(size <= MAX_BYTES);

	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(obj);

	// 如果自由链表的长度大于申请一个批量的长度就还给central cache 
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		ListTooLong(_freeLists[index], size);
	}
}

void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* start = nullptr;
	void* end = nullptr;

	// 先将MaxSize长度的内存，从thread cache层释放出来
	list.PopRange(start, end, list.MaxSize());

	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}
