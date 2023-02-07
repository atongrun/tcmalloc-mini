#include "ThreadCache.h"

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

//�����Ļ����ȡ����
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	return nullptr;
}
