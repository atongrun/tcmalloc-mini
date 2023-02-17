#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

static void* ConcurrentAlloc(size_t size)
{
	if (size > MAX_BYTES)
	{
		// 申请的内存大于MAX_BYTES(256kb)
		// 走Page Cache层申请

		// 先进行对齐
		size_t alignSize = SizeClass::RoundUp(size);
		
		// 然后算需要几页
		size_t kpage = alignSize >> PAGE_SHITF;
		PageCache::GetInstance()->_pageMtx.lock();
		Span* span = PageCache::GetInstance()->NewSpan(alignSize);
		span->_ObjSize = alignSize;
		PageCache::GetInstance()->_pageMtx.unlock();

		return (void*)(span->_pageId << PAGE_SHITF);
	}
	else
	{
		if (pTLSThreadCache == nullptr)
		{
			/*pTLSThreadCache = new ThreadCache;*/
			static ObjectPool<ThreadCache> tcPool;
			pTLSThreadCache = tcPool.New();
		}

		// 测试代码，获取进程的id
		//cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl;

		return pTLSThreadCache->Allocate(size);
	}
}


static void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t size = span->_ObjSize;

	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->_pageMtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_pageMtx.unlock();

	}
	else
	{
		assert(pTLSThreadCache);
		pTLSThreadCache->Deallocate(ptr, size);
	}
}