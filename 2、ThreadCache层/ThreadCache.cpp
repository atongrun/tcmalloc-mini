#include "ThreadCache.h"

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
}

//从中心缓存获取对象
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	return nullptr;
}
