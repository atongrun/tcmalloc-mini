#pragma once

#include "Common.h"


class PageCache
{
public:
	//整个进程只有一份，同样设计成单例模式
	static PageCache* GetInstance()
	{
		return &_sInst;
	}
	// 要一个k页的span
	Span* NewSpan(size_t k);
public:
	std::mutex _pageMtx;
private:
	PageCache() {};
	PageCache(const PageCache&) = delete;
	SpanList _spanLists[NPAGES];

	static PageCache _sInst;
};