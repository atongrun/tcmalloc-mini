#pragma once
#include "Common.h"

class ThreadCache
{
public:
	// �����ڴ�
	void* Allocate(size_t size);
	// �ͷ��ڴ�
	void Deallocate(void* obj, size_t size);

	//�����Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t size);
private:
	// ��������ڴ��С����������Ĺ�ϣͰ
	FreeList _freeLists[NFREELIST];
};


// TLS thread local storage
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;