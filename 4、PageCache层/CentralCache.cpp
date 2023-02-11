#include "CentralCache.h"
#include "PageCache.h"
CentralCache CentralCache::_sInst; //����ʵ�֣����ü�static


// ��ȡһ���ǿյ�span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	// 1����ȥsize ��Ӧ��Ͱ���ҷǿյ�span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}

	// ��������Ƚ����Ͱ��Ͱ�����н����������߳̿����ͷ��ڴ���������������
	list._mtx.unlock();

	// ������˵����size��Ӧ��CentralCache������û��Span������ô��Ҫ����һ������
	// �Ƚ�������page cache��ļ���
	PageCache::GetInstance()->_pageMtx.lock();
	
	// ����Ҫ������page cache��Ҫ����ҳ��span
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));

	// ����
	PageCache::GetInstance()->_pageMtx.unlock();


	// ���������span��������з֣���᲻��Ҫ��������Ϊ��������̷߳��ʲ������span����û�ҵ�spanLists��
	
	// Ҫ����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С
	// ҳ������PAGE_SHIFT���ǵ�ַ
	char* start = (char*)(span->_pageId << PAGE_SHITF);
	
	size_t bytes = span->_n << PAGE_SHITF;
	char* end = start + bytes;
	

	// ����һ����ͷ
	span->_freeList = start;
	start += size;
		
	// ����β��
	void* tail = span->_freeList;

	while(start <end)
	{
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}

	// �к�span����span�ҵ�Ͱ�����
	list._mtx.lock();
	list.PushFront(span);

	return span;
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
	
	// �������ͷŵ�ʱ����
	span->_useCount += actualNum;
	// ����
	_spanLists[index]._mtx.unlock();

	return actualNum;
}