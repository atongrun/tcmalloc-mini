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


// thread cache申请的最大字节数
static const size_t MAX_BYTES = 256 * 1024;

// thread cache和central cache 哈希桶的个数-按对齐规则算是208个桶
static const size_t NFREELIST = 208;

// page cache桶的个数 - 这里设置为129个，0号桶不用
static const size_t NPAGES = 129;

// 一页是8k，就是2^13次方
static const size_t PAGE_SHITF = 13;

//x64  _WIN32和_WIN64都有定义
//win32 _WIN32有定义，_WIN64没有定义
#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
//#elif CONFIG_X86_32
//	typedef size_t PAGEID;
//#elif CONFIG_X86_64
//	typedef unsigned long long PAGE_ID;
#endif // _WIN64

	// 直接去堆上按页申请空间
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux下brk mmap等
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}




static void*& NextObj(void* obj)
{
	return *(void**)obj;
}

// 管理切分好的小对象的自由链表
class FreeList
{
public:
	void Push(void* obj)
	{
		assert(obj);
		//头插
		NextObj(obj) = _freeList;
		_freeList = obj;
	}

	void* Pop()
	{
		assert(_freeList); 
		//头删
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




// 计算对象大小的对齐映射规则
// 1、需要将对应的字节数先向上取整
// 2、取整后的字节数转换到哈希下标
class SizeClass
{
public:
	// 整体控制在最多10%左右的内碎片浪费
	// [1,128]					8byte对齐	    freelist[0,16)
	// [128+1,1024]				16byte对齐	    freelist[16,72)
	// [1024+1,8*1024]			128byte对齐	    freelist[72,128)
	// [8*1024+1,64*1024]		1024byte对齐     freelist[128,184)
	// [64*1024+1,256*1024]		8*1024byte对齐   freelist[184,208)

	static inline size_t _RoundUp(size_t bytes, size_t alignNum)
	{
		//// 常规写法
		//if (bytes % alignNum == 0)
		//{
		//	return bytes;
		//}
		//else
		//{
		//	// 9 对齐到16 | 9/8+1=2 2*8=16
		//	// 就是看当前的size占几个对齐数，然后+1 再乘对齐数
		//	return (bytes / alignNum + 1) * alignNum;
		//}

		// 牛比写法
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
			//说明出现了不想申请的字节数，直接报错
			assert(false);
			return -1;
		}
	}


	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	// 计算映射的哪一个自由链表桶
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);

		// 每个区间有多少个链
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

	// 一次thread cache从中心节点获取多少个
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);
		size_t num = MAX_BYTES / size;

		//一次批量移动多少个对象的(慢启动)上限值为[2,512]
		// 小对象一次批量上限高
		// 大对象一次批量上限低
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;
		return num;
	}

	// 计算一次向系统获取多少个页

	static size_t NumMovePage(size_t size)
	{
		// 先获取申请size对象大小的上限(尽可能多要一些)
		size_t num = NumMoveSize(size);

		// 算一下num个对象总共多少字节，然后右移13位就是多少页
		size_t npage = (num * size) >> PAGE_SHITF;
		// 最少申请1页
		if (npage == 0)
			npage = 1;
		return npage;
	}
};



// 管理多个连续页的大块内存跨度结构

struct Span
{
	//假设一页为8k
	//32位程序 2^32/2^13 
	//64

	PAGE_ID _pageId = 0; //大块内存起始页的页号
	size_t _n = 0; //页的数量

	Span* _next = nullptr; //双向链表的结构
	Span* _prev = nullptr;

	size_t _useCount = 0; //切好的小块内存，被分配给thread cache的计数

	void* _freeList =nullptr; //切好小块内存的自由链表
};

// 带头双向的循环链表
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
		// 先断言一下有没有问题
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
	// 头删返回
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
	std::mutex _mtx; //桶锁

};