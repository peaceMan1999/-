#define _CRT_SECURE_NO_WARNINGS 1
#include "CentralCache.h"

CentralCache CentralCache::_cc;


Span* CentralCache::GetOneSpan(SpanList& list, size_t bytes) // ����SpanΪ�գ���ȡSpan
{
	return nullptr;
}


size_t CentralCache::GetRangeObj(void*& start, void*& end, size_t batchNum, size_t bytes) // ʵ�ʸ�TC������
{
	size_t index = SizeClass::Index(bytes);
	_CCSpanList[index]._mtx.lock(); // Ͱ����
	// TODO
	Span* span = GetOneSpan(_CCSpanList[index], bytes); // Ҫ���ڴ棬��������ڵ����в���
	assert(span);
	assert(span->_SpanFreeList);

	start = span->_SpanFreeList;
	end = start; // end�ȵ���start��Ȼ����������

	size_t i = 0, actualNum = 1;
	while (i < batchNum-1 && NextObj(end) != nullptr) // -1 ����Ϊend
	{
		end = NextObj(end); // ������
		i++;
		actualNum++;
	}
	span->_SpanFreeList = NextObj(end); // ���ߺ��ʣ�µĵ�һ������
	NextObj(end) = nullptr; // �����ߵ����һ����_next�ÿ�

	// ..��������ô��

	_CCSpanList[index]._mtx.unlock(); // ����

	return actualNum;
}

