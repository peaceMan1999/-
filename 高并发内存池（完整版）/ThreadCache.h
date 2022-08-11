#pragma once
#include "Common.h"

class ThreadCache
{
public:
	// ����
	void* Allocate(size_t bytes); // �����ڴ�

	void Deallocate(void* ptr, size_t bytes); //�ͷ��ڴ�

	void* GetFromCentalCache(size_t index, size_t bytes); // �������������Ļ�ȡ�ڴ�

	void ListToLong(FreeList& list, size_t bytes); // ����Ҫ����

private:
	FreeList _TCFreeList[MAXBUCKETS]; // ����_freeList��������208����ϣͰ��0~207;
};

// TLS����̼߳���������
// �������ֻ���Լ��ܿ��������̲߳��ܿ�
static _declspec(thread) ThreadCache* pTSLThreadCache = nullptr;