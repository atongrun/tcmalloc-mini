#pragma once
#include <iostream>
#include <vector>
#include <time.h>
using std::cout;
using std::endl;
#ifdef _WIN32
	#include<Windows.h>
#else
	#include <unistd.h>
#endif // _WIN32

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
				_remainBytes = 128 * 1024;
				_memory = (char*)malloc(_remainBytes);
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


struct TreeNode
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;

	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};

void TestObjectPool()
{
	// �����ͷŵ��ִ�
	const size_t Rounds = 5;

	// ÿ�������ͷŶ��ٴ�
	const size_t N = 1000000;

	std::vector<TreeNode*> v1;
	v1.reserve(N);

	size_t begin1 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v1.push_back(new TreeNode);
		}
		for (int i = 0; i < N; ++i)
		{
			delete v1[i];
		}
		v1.clear();
	}

	size_t end1 = clock();

	std::vector<TreeNode*> v2;
	v2.reserve(N);

	ObjectPool<TreeNode> TNPool;
	size_t begin2 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v2.push_back(TNPool.New());
		}
		for (int i = 0; i < N; ++i)
		{
			TNPool.Delete(v2[i]);
		}
		v2.clear();
	}
	size_t end2 = clock();

	cout << "new����ʱ��:" << end1 - begin1 << endl;
	cout << "�����ڴ�ػ���ʱ��:" << end2 - begin2 << endl;
}