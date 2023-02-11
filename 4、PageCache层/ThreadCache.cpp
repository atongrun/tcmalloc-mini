#include "ThreadCache.h"
#include "CentralCache.h"

//�����Ļ����ȡ����
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	// ����ʼ���������㷨
	// ����Ҫ���ٸ�size��С�Ķ���
	// ����ע�⣬Windows.h���и���min�������std���ͻ����
	size_t batchNum = min(SizeClass::NumMoveSize(size), _freeLists[index].MaxSize());

	if (_freeLists[index].MaxSize() == batchNum)
	{
		_freeLists[index].MaxSize() += 1;
	}


	// ֪��Ҫ���ٸ���������central cache��ҪbatchNum������
	void* start;
	void* end;

	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
	assert(actualNum >= 1);

	if (actualNum == 1)
	{
		//ֻ��һ��
		assert(start == end);
		return start;
	}
	else
	{
		// ��NextObj(start)-end ͷ���������������start
		_freeLists[index].PushRange(NextObj(start), end);
		return start;
	}

}


// �����ڴ�
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES); 
	// ����size���ֽ�ӳ�䵽�ĸ�Ͱ�����ǲ�ͬ���ֽڷ�Χ��ӳ��Ĺ���ͬ����һ�������

	//�ҵ���ǰ�ֽ����Ķ��������Լ���Ӧ��ϣͰ���±�
	size_t alignSize = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);

	// ���������������Ϊ�գ�ͷɾ����ͷ�ڵ�
	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		//Ϊ�գ���ȥCentralCache���ȡ
		return FetchFromCentralCache(index, alignSize);
	}

}
// �ͷ��ڴ�
void ThreadCache::Deallocate(void* obj, size_t size)
{
	assert(obj);
	assert(size <= MAX_BYTES);

	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(obj);
}

