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

	// 获取对象到span的映射
	Span* MapObjectToSpan(void* obj);

	// 释放空闲span回到PageCache，并合并相邻的span
	void ReleaseSpanToPageCache(Span* span);

public:
	std::mutex _pageMtx;
private:
	PageCache() {};
	PageCache(const PageCache&) = delete;

	std::unordered_map<PAGE_ID, Span*> _idSpanMap;

	SpanList _spanLists[NPAGES];

	static PageCache _sInst;
};