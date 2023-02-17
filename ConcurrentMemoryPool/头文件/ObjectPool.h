#pragma once
#include <iostream>

#ifdef _WIN32
#include<Windows.h>
#else
#include <unistd.h>
#endif


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

// 系统调用释放内存
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap等
#endif
}

template<class T>
class ObjectPool
{
public:
	T* New()
	{
		T* obj = nullptr;
		if (_freeList)
		{
			// 直接进行头删
			obj = (T*)_freeList;
			_freeList = *(void**)_freeList;
		}
		else
		{
			//说明需要先申请大块内存

			if (_remainBytes < sizeof(T))
			{
				//说明需要先申请大块内存
				/*_remainBytes = 128 * 1024;
				_memory = (char*)malloc(_remainBytes);*/
				_remainBytes = 128 * 1024;
				_memory = (char*)SystemAlloc(_remainBytes >> 13);
			}
			size_t objSize = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);
			obj = (T*)_memory;
			_memory += objSize;
			_remainBytes -= objSize;
		}


		//对T类型初始化-使用定位new
		new(obj)T;
		return obj;
	}

	void Delete(T* obj)
	{
		if (obj == nullptr)
		{
			return;
		}
		else
		{
			//先显示调用析构
			obj->~T();
			//头插 进freeList链表
			*(void**)obj = _freeList;
			_freeList = (void*)obj;
		}
	}
private:
	char* _memory = nullptr; // 指向大块内存的指针
	size_t _remainBytes = 0; // 大块内存在切分过程中剩余字节数

	void* _freeList = nullptr; // 还回来过程中链接的自由链表的头指针
};


//struct TreeNode
//{
//	int _val;
//	TreeNode* _left;
//	TreeNode* _right;
//
//	TreeNode()
//		:_val(0)
//		, _left(nullptr)
//		, _right(nullptr)
//	{}
//};
//
//void TestObjectPool()
//{
//	// 申请释放的轮次
//	const size_t Rounds = 5;
//
//	// 每轮申请释放多少次
//	const size_t N = 1000000;
//
//	std::vector<TreeNode*> v1;
//	v1.reserve(N);
//
//	size_t begin1 = clock();
//	for (size_t j = 0; j < Rounds; ++j)
//	{
//		for (int i = 0; i < N; ++i)
//		{
//			v1.push_back(new TreeNode);
//		}
//		for (int i = 0; i < N; ++i)
//		{
//			delete v1[i];
//		}
//		v1.clear();
//	}
//
//	size_t end1 = clock();
//
//	std::vector<TreeNode*> v2;
//	v2.reserve(N);
//
//	ObjectPool<TreeNode> TNPool;
//	size_t begin2 = clock();
//	for (size_t j = 0; j < Rounds; ++j)
//	{
//		for (int i = 0; i < N; ++i)
//		{
//			v2.push_back(TNPool.New());
//		}
//		for (int i = 0; i < N; ++i)
//		{
//			TNPool.Delete(v2[i]);
//		}
//		v2.clear();
//	}
//	size_t end2 = clock();
//
//	cout << "new花费时间:" << end1 - begin1 << endl;
//	cout << "定长内存池花费时间:" << end2 - begin2 << endl;
//}
//
