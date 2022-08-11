#pragma once
#include "Common.h"
#include "PageCache.h"
#include "ThreadCache.h"

static void* ConcurrentAlloc(size_t bytes)
{
	if (bytes > MAXBYTES)
	{
		// ֱ���ڶ�������
		size_t alignSize = SizeClass::RoundUp(bytes);
		size_t page = alignSize >> PAGESHIFT; // ҳ��

		PageCache::GetInstance()->PC_mtx.lock();
		Span* span = PageCache::GetInstance()->NewSpan(page);
		span->_objectSize = bytes;
		PageCache::GetInstance()->PC_mtx.unlock();

		void* ptr = (void*)(span->_pageId << PAGESHIFT);
		return ptr;
	}
	else
	{
		if (pTSLThreadCache == nullptr)
		{	//˵����û�����������ڴ棬ͨ��TSL�����ػ�ȡ�Լ���ThreadCache����
			static ObjectPool<ThreadCache> TCPool;
			pTSLThreadCache = TCPool.New();
		}
		//cout << std::this_thread::get_id() << " : " << pTSLThreadCache << endl;

		return pTSLThreadCache->Allocate(bytes); // �����Լ��ĳ�Ա��������ռ�
	}
}

static void ConcurrentFree(void* ptr)
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	size_t bytes = span->_objectSize;

	if (bytes > MAXBYTES)
	{
		PageCache::GetInstance()->PC_mtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPC(span);
		PageCache::GetInstance()->PC_mtx.unlock();
	}
	else
	{
		assert(pTSLThreadCache); // �ж��Ƿ�����
		pTSLThreadCache->Deallocate(ptr, bytes);
	}
}
