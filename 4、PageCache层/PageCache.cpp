#include "PageCache.h"

PageCache PageCache::_sInst;

// page cache ����һ��span
Span* PageCache::NewSpan(size_t k)
{
	// �ȶ���һ��
	assert(k > 0 && k < NPAGES);

	// �ȼ���k��Ͱ����û��Span
	if (!_spanLists[k].Empty())
	{
		// ͷɾ��һ����������ȥʵ��PopFront���߼�
		return _spanLists[k].PopFront();
	}

	//���k��ͰΪ�գ��������Ͱ���ҵ�һ���ǿ�Ͱ��ֱ�����һ��Ͱ
	for (size_t i = k + 1; i < NPAGES; i++)
	{
		if (!_spanLists[i].Empty())
		{
			Span* nSpan = _spanLists[i].PopFront();
			// ��
			Span* kSpan = new Span;

			// �ֳ�npan ��һ�� kҳ���أ�Ȼ��n-kҳ��span�ҵ�n-k��Ͱ��
			// ����ע�⣬ֻ��Ҫ��ҳ�ź�ҳ���Ϳ����ˣ�span�ҵ��ڴ�ĵ�ַ��ҳ��ȡ
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_n -= k;
			nSpan->_pageId += k;


			_spanLists[nSpan->_n].PushFront(nSpan);
			return kSpan;
		}
	}

	// ������˵�����������λ�ö�û�д�ҳ��span
	// ��ʱ�����Ҫ�������һ��128ҳ��span
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(NPAGES-1);


	bigSpan->_n = NPAGES - 1;
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHITF;

	_spanLists[bigSpan->_n].PushFront(bigSpan);

	return NewSpan(k);
}
