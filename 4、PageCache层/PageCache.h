#pragma once

#include "Common.h"


class PageCache
{
public:
	//��������ֻ��һ�ݣ�ͬ����Ƴɵ���ģʽ
	static PageCache* GetInstance()
	{
		return &_sInst;
	}
	// Ҫһ��kҳ��span
	Span* NewSpan(size_t k);
public:
	std::mutex _pageMtx;
private:
	PageCache() {};
	PageCache(const PageCache&) = delete;
	SpanList _spanLists[NPAGES];

	static PageCache _sInst;
};