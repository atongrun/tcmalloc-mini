#pragma once
#include "Common.h"

class ThreadCache
{
public:
	// 申请内存
	void* Allocate(size_t size);
	// 释放内存
	void Deallocate(void* obj, size_t size);

	//从中心缓存获取对象
	void* FetchFromCentralCache(size_t index, size_t size);
private:
	// 管理各种内存大小的自由链表的哈希桶
	FreeList _freeLists[NFREELIST];
};


// TLS thread local storage
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;