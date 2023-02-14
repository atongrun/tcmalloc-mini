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

	// ��ȡ����span��ӳ��
	Span* MapObjectToSpan(void* obj);

	// �ͷſ���span�ص�PageCache�����ϲ����ڵ�span
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