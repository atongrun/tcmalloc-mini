#include "CentralCache.h"

CentralCache CentralCache::_sInst; //����ʵ�֣����ü�static


// ��ȡһ���ǿյ�span
Span* CentralCache::GetOneSpan(SpanList& list, size_t byte_size)
{
	// ...
	return nullptr;
}

// �����Ļ����ȡһ�������Ķ����thread cache
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	// ����size��С�Ķ��󣬶�Ӧ�ĸ�Ͱ��Ȼ���ȥ�ĸ�Ͱȥ����
	size_t index = SizeClass::Index(size);
	// ����-��Ͱ��
	_spanLists[index]._mtx.lock();

	// ���ȱ�֤��һ���ǿյ�Span
	Span* span = GetOneSpan(_spanLists[index], size);
	assert(span);
	assert(span->_freeList);

	// ��span�л�ȡbatchNum������
	// �������batchNum���м����ü���������ʵ���õ��ĸ���

	start = span->_freeList;
	end = start;
	int actualNum = 1;
	size_t i = 0;
	while (i<batchNum && NextObj(end) != nullptr)
	{
		end = NextObj(end);
		++i;
		++actualNum;
	}

	// ���� actualNum������
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;
	
	// ����
	_spanLists[index]._mtx.unlock();

	return actualNum;
}