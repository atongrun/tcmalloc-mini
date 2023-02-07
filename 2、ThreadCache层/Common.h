#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <time.h>
#include <assert.h>
using std::cout;
using std::endl;

// thread cache���������ֽ���
static const size_t MAX_BYTES = 256 * 1024;

// ��ϣͰ�ĸ���-�������������208��Ͱ
static const size_t NFREELIST = 208;

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

	bool Empty()
	{
		return _freeList == nullptr;
	}
private:
	void* _freeList = nullptr;
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
};