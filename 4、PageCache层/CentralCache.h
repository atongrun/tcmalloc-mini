#pragma once
#include "Common.h"

// 单例模式
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	// 获取一个非空的span
	Span* GetOneSpan(SpanList& list, size_t byte_size);

	// 从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);
private:
	static CentralCache _sInst;
private:
	CentralCache(){} //构造函数私有化
	CentralCache(const CentralCache& c) = delete; //禁用拷贝
private:
	SpanList _spanLists[NFREELIST]; // SpanList数组（哈希桶）
};