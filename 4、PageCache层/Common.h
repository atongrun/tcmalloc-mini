#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <time.h>
#include <assert.h>
#include <mutex>
#include <algorithm>

using std::cout;
using std::endl;

#ifdef _WIN32
	#include<Windows.h>
#else
	#include <unistd.h>
#endif


// thread cache���������ֽ���
static const size_t MAX_BYTES = 256 * 1024;

// thread cache��central cache ��ϣͰ�ĸ���-�������������208��Ͱ
static const size_t NFREELIST = 208;

// page cacheͰ�ĸ��� - ��������Ϊ129����0��Ͱ����
static const size_t NPAGES = 129;

// һҳ��8k������2^13�η�
static const size_t PAGE_SHITF = 13;

//x64  _WIN32��_WIN64���ж���
//win32 _WIN32�ж��壬_WIN64û�ж���
#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
//#elif CONFIG_X86_32
//	typedef size_t PAGEID;
//#elif CONFIG_X86_64
//	typedef unsigned long long PAGE_ID;
#endif // _WIN64

	// ֱ��ȥ���ϰ�ҳ����ռ�
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux��brk mmap��
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}




static void*& NextObj(void* obj)
{
	return *(void**)obj;
}

// �����зֺõ�С�������������
class FreeList
{
public:
	void Push(void* obj)
	{
		assert(obj);
		//ͷ��
		NextObj(obj) = _freeList;
		_freeList = obj;
	}

	void* Pop()
	{
		assert(_freeList); 
		//ͷɾ
		void* obj = _freeList;
		_freeList = NextObj(_freeList);
		return obj;
	}

	void PushRange(void* start, void* end)
	{
		NextObj(end) = _freeList;
		_freeList = start;
	}
	size_t& MaxSize()
	{
		return _maxSize;
	}
	bool Empty()
	{
		return _freeList == nullptr;
	}
private:
	void* _freeList = nullptr;
	size_t _maxSize = 1;
};




// ��������С�Ķ���ӳ�����
// 1����Ҫ����Ӧ���ֽ���������ȡ��
// 2��ȡ������ֽ���ת������ϣ�±�
class SizeClass
{
public:
	// ������������10%���ҵ�����Ƭ�˷�
	// [1,128]					8byte����	    freelist[0,16)
	// [128+1,1024]				16byte����	    freelist[16,72)
	// [1024+1,8*1024]			128byte����	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte����     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte����   freelist[184,208)

	static inline size_t _RoundUp(size_t bytes, size_t alignNum)
	{
		//// ����д��
		//if (bytes % alignNum == 0)
		//{
		//	return bytes;
		//}
		//else
		//{
		//	// 9 ���뵽16 | 9/8+1=2 2*8=16
		//	// ���ǿ���ǰ��sizeռ������������Ȼ��+1 �ٳ˶�����
		//	return (bytes / alignNum + 1) * alignNum;
		//}

		// ţ��д��
		return ((bytes + alignNum - 1) & ~(alignNum - 1));
	}
	static inline size_t RoundUp(size_t size)
	{
		if (size <= 128)
		{
			return _RoundUp(size, 8);
		}
		else if (size <= 1024)
		{
			return _RoundUp(size, 16);

		}
		else if (size <= 8 * 1024)
		{
			return _RoundUp(size, 128);

		}
		else if (size <= 64 * 1024)
		{
			return _RoundUp(size, 1024);

		}
		else if (size <= 256 * 1024)
		{
			return _RoundUp(size, 8*1024);

		}
		else
		{
			//˵�������˲���������ֽ�����ֱ�ӱ���
			assert(false);
			return -1;
		}
	}


	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	// ����ӳ�����һ����������Ͱ
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		// ÿ�������ж��ٸ���
		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128) {
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8 * 1024) {
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
		}
		else {
			assert(false);
		}

		return -1;
	}

	// һ��thread cache�����Ľڵ��ȡ���ٸ�
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);
		size_t num = MAX_BYTES / size;

		//һ�������ƶ����ٸ������(������)����ֵΪ[2,512]
		// С����һ���������޸�
		// �����һ���������޵�
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;
		return num;
	}

	// ����һ����ϵͳ��ȡ���ٸ�ҳ

	static size_t NumMovePage(size_t size)
	{
		// �Ȼ�ȡ����size�����С������(�����ܶ�ҪһЩ)
		size_t num = NumMoveSize(size);

		// ��һ��num�������ܹ������ֽڣ�Ȼ������13λ���Ƕ���ҳ
		size_t npage = (num * size) >> PAGE_SHITF;
		// ��������1ҳ
		if (npage == 0)
			npage = 1;
		return npage;
	}
};



// ����������ҳ�Ĵ���ڴ��Ƚṹ

struct Span
{
	//����һҳΪ8k
	//32λ���� 2^32/2^13 
	//64

	PAGE_ID _pageId = 0; //����ڴ���ʼҳ��ҳ��
	size_t _n = 0; //ҳ������

	Span* _next = nullptr; //˫������Ľṹ
	Span* _prev = nullptr;

	size_t _useCount = 0; //�кõ�С���ڴ棬�������thread cache�ļ���

	void* _freeList =nullptr; //�к�С���ڴ����������
};

// ��ͷ˫���ѭ������
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	bool Empty()
	{
		return _head == _head->_next;
	}
	void Insert(Span* pos, Span* newSpan)
	{
		// �ȶ���һ����û������
		assert(pos);
		assert(newSpan);

		Span* prev = pos->_prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;

		prev->_next = newSpan;
		newSpan->_prev = prev;
	}
	
	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}
	// ͷɾ����
	Span* PopFront()
	{
		Span* sp = _head->_next;
		/*head->_next = sp->_next;
		sp->_next = nullptr;
		return sp;*/
		Erase(sp);
		return sp;
	}

	void Erase(Span *pos)
	{
		assert(pos);
		assert(pos != _head);

		Span* prev = pos->_prev;
		Span* next = pos->_next;

		next->_prev = prev;
		prev->_next = next;
	}
private:
	Span* _head;
public:
	std::mutex _mtx; //Ͱ��

};