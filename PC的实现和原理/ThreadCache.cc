#define _CRT_SECURE_NO_WARNINGS 1
#include "ThreadCache.h"
#include "CentralCache.h"


void* ThreadCache::GetFromCentalCache(size_t index, size_t bytes) // �������������Ļ�ȡ�ڴ�
{
	// ����������
	size_t batchNum = min(_TCFreeList[index].MaxSize(), SizeClass::NumMoveSize(bytes));// �����С
	// ��������
	if (batchNum == _TCFreeList[index].MaxSize())
	{
		_TCFreeList[index].MaxSize() += 1; // ��->��		
	}
	void* start = nullptr;
	void* end = nullptr;
	// ��ȡʵ�ʴ�С
	size_t actualNum = CentralCache::GetInstance()->GetRangeObj(start, end, batchNum, bytes);
	assert(actualNum > 0);

	if (actualNum == 1) // ����ͷ
	{
		assert(start == end);
		return start;
	}
	else
	{
		_TCFreeList[index].PushRange(NextObj(start), end); // ����ȷ��ţ��ȸ�һ��
		return start;
	}
}

void* ThreadCache::Allocate(size_t bytes) // �����ڴ�
{
	assert(bytes <= MAXBYTES);
	size_t alignSize = SizeClass::RoundUp(bytes); // ��ȡʵ�ʶ���bytes
	size_t index = SizeClass::Index(bytes);  // �ĸ�Ͱ

	if (!_TCFreeList[index].Empty())
	{
		// ��Ϊ�վͻ�ȡ�ڴ�
		return _TCFreeList[index].Pop();
	}
	else 
	{
		// �վ������������ڴ�
		return GetFromCentalCache(index, alignSize);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t bytes) //�ͷ��ڴ�
{
	assert(ptr);
	assert(bytes <= MAXBYTES);

	size_t index = SizeClass::Index(bytes);  // �ĸ�Ͱ

	// ��ӻ���������
	_TCFreeList[index].Push(ptr);
}


