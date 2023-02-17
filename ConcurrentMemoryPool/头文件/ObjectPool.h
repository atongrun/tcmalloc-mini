#pragma once
#include <iostream>

#ifdef _WIN32
#include<Windows.h>
#else
#include <unistd.h>
#endif


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

// ϵͳ�����ͷ��ڴ�
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap��
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
			// ֱ�ӽ���ͷɾ
			obj = (T*)_freeList;
			_freeList = *(void**)_freeList;
		}
		else
		{
			//˵����Ҫ���������ڴ�

			if (_remainBytes < sizeof(T))
			{
				//˵����Ҫ���������ڴ�
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


		//��T���ͳ�ʼ��-ʹ�ö�λnew
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
			//����ʾ��������
			obj->~T();
			//ͷ�� ��freeList����
			*(void**)obj = _freeList;
			_freeList = (void*)obj;
		}
	}
private:
	char* _memory = nullptr; // ָ�����ڴ��ָ��
	size_t _remainBytes = 0; // ����ڴ����зֹ�����ʣ���ֽ���

	void* _freeList = nullptr; // ���������������ӵ����������ͷָ��
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
//	// �����ͷŵ��ִ�
//	const size_t Rounds = 5;
//
//	// ÿ�������ͷŶ��ٴ�
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
//	cout << "new����ʱ��:" << end1 - begin1 << endl;
//	cout << "�����ڴ�ػ���ʱ��:" << end2 - begin2 << endl;
//}
//
