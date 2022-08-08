#define _CRT_SECURE_NO_WARNINGS 1
#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_cc;


Span* CentralCache::GetOneSpan(SpanList& list, size_t bytes) // ����SpanΪ�գ���ȡSpan
{
	// �ȱ�������������span���о���
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_SpanFreeList != nullptr) // �о�����
		{
			return it;
		}
		else // û�о���һ��
		{
			it = it->_next;
		}
	}

	// Ҫ�Ƚ⿪Ͱ��
	list.CC_mtx.unlock();
	// �ߵ���˵��CCû�У���ʼ��PC��span������һ����ڴ棬����װ���������ģ�������Ҫ��װ�������ü�������
	PageCache::GetInstance()->PC_mtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(bytes));
	PageCache::GetInstance()->PC_mtx.unlock();

	// ��ʼλ��
	char* start = (char*)(span->_pageId << PAGESHIFT);
	size_t BigBytes = span->_n << PAGESHIFT; // ҳ��*8k
	char* end = start + BigBytes;  // �����������������С
	// ��ʼ��װ����һ����ڴ���װ��һ����������

	span->_SpanFreeList = start;
	void* tail = start;
	start += bytes;
	int i = 1;
	while (start < end)
	{
		NextObj(tail) = start;
		tail = start;
		start += bytes;
		i++;
	}
	cout << i << endl;

	// ͷ���CC��byte��
	list.CC_mtx.lock();
	list.PushFront(span);

	return span;
}


size_t CentralCache::GetRangeObj(void*& start, void*& end, size_t batchNum, size_t bytes) // ʵ�ʸ�TC������
{
	size_t index = SizeClass::Index(bytes);
	_CCSpanList[index].CC_mtx.lock(); // ����Ͱ��ʱ�����
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

	span->_usecount += actualNum;  // ���߼�����+����
	// ..��������ô��

	_CCSpanList[index].CC_mtx.unlock(); // ����

	return actualNum;
}

