#pragma once
#include "Common.h"
#include "ThreadCache.h"

static void* ConcurrentAlloc(size_t bytes)
{
	if (pTSLThreadCache == nullptr)
	{	//˵����û�����������ڴ棬ͨ��TSL�����ػ�ȡ�Լ���ThreadCache����
		pTSLThreadCache = new ThreadCache;
	}

	//cout << std::this_thread::get_id() << " : " << pTSLThreadCache << endl;

	return pTSLThreadCache->Allocate(bytes); // �����Լ��ĳ�Ա��������ռ�
}

static void ConcurrentFree(void* ptr, size_t bytes)
{
	assert(pTSLThreadCache); // �ж��Ƿ�����

	pTSLThreadCache->Deallocate(ptr, bytes);
}
