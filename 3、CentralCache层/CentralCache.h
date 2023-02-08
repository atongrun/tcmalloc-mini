#pragma once
#include "Common.h"

// ����ģʽ
class CentralCache
{
public:
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	// ��ȡһ���ǿյ�span
	Span* GetOneSpan(SpanList& list, size_t byte_size);

	// �����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);
private:
	static CentralCache _sInst;
private:
	CentralCache(){} //���캯��˽�л�
	CentralCache(const CentralCache& c) = delete; //���ÿ���
private:
	SpanList _spanLists[NFREELIST]; // SpanList���飨��ϣͰ��
};