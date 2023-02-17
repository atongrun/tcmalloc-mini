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
	span->_ObjSize = size;


	span->_isUse = true;

	// ����
	PageCache::GetInstance()->_pageMtx.unlock();


	// ���������span��������з֣���᲻��Ҫ��������Ϊ��������̷߳��ʲ������span����û�ҵ�spanLists��
	
	// Ҫ����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С
	// ҳ������PAGE_SHIFT���ǵ�ַ
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	
	size_t bytes = span->_n << PAGE_SHIFT;
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
	// ?? ע�� β�ڵ�Ҫ�ÿ�
	//  ...
	
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


void CentralCache::ReleaseListToSpans(void* start, size_t size)
{
	size_t index = SizeClass::Index(size);

	_spanLists[index]._mtx.lock();

	// ���⣺���������ڴ�ҵ��ĸ�span�ϣ�
	// ͨ����ַ start>>PAGE_SHIFT������span��ҳ�ţ�Ϊ�˷��㣬���ǿ��Խ���ҳ�ź�spanָ���ӳ��
	// �������ͨ����ϣ����- unordered_map<PAGE_ID, Span*> 
	while (start)
	{
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);
		
		// ͷ���span�Ĺ��������
		NextObj(start) = span->_freeList;
		span->_freeList = start;

		span->_useCount--;

		// span�зֻ�ȥ��С���ڴ涼������
		// �����span ���ո�page cache
		if (span->_useCount == 0)
		{
			_spanLists[index].Erase(span);
			span->_next = nullptr;
			span->_next = nullptr;
			// span�����ڴ棬ͨ��ҳ�ź�ҳ���Ϳ���
			span->_freeList = nullptr;

			_spanLists[index]._mtx.unlock();

			PageCache::GetInstance()->_pageMtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);

			PageCache::GetInstance()->_pageMtx.unlock();

			_spanLists[index]._mtx.lock();
		}

		start = next;

	}
	//����
	_spanLists[index]._mtx.unlock();
}
