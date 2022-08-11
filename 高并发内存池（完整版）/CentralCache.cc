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
	span->_IsUse = true;
	span->_objectSize = bytes;
	PageCache::GetInstance()->PC_mtx.unlock();

	// ��ʼλ��
	char* start = (char*)(span->_pageId << PAGESHIFT);
	size_t BigBytes = span->_n << PAGESHIFT; // ҳ��*8k
	char* end = start + BigBytes;  // �����������������С
	// ��ʼ��װ����һ����ڴ���װ��һ����������

	span->_SpanFreeList = start;
	start += bytes;
	void* tail = span->_SpanFreeList;
	//int i = 1;
	while (start < end)
	{
		NextObj(tail) = start;
		tail = start;
		start += bytes;
		//i++;
	}
	//cout << i << endl;

	NextObj(tail) = nullptr;

	//size_t j = 0;
	//void* cur = span->_SpanFreeList;
	//while (cur)
	//{
	//	cur = NextObj(cur);
	//	j++;
	//}
	//if (j != BigBytes / bytes)
	//{
	//	int x = 0;
	//}

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
	
	//size_t j = 0;
	//void* cur = start;
	//while (cur)
	//{
	//	cur = NextObj(cur);
	//	j++;
	//}
	//if (j != actualNum)
	//{
	//	int x = 0;
	//}

	_CCSpanList[index].CC_mtx.unlock(); // ����

	return actualNum;
}

void CentralCache::ReleaseListToSpan(void* start, size_t bytes) // �����TC���յ���������
{
	size_t index = SizeClass::Index(bytes);
	
	// ����CC[index]�ϵ���һ��span�أ���MapObjectToSpan�ҵ���һҳ�ϵ�span
	_CCSpanList[index].CC_mtx.lock();
	while (start)
	{
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start); // �ҵ���Ӧӳ���span��
		// ͷ��,˫��
		NextObj(start) = span->_SpanFreeList;
		span->_SpanFreeList = start;
		span->_usecount--; // �黹��һ

		// usecount == 0˵���зֵĶ������ˣ����Ի���PC�ϲ�ǰ��ҳ
		if (span->_usecount == 0)
		{
			_CCSpanList[index].Erase(span); // �õ�
			span->_SpanFreeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;
			// Ҫ�����𣿲��ã����忴����
			_CCSpanList[index].CC_mtx.unlock(); // �⿪Ͱ������ȫ��
			PageCache::GetInstance()->PC_mtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPC(span);
			PageCache::GetInstance()->PC_mtx.unlock();
			_CCSpanList[index].CC_mtx.lock();

		}
		start = next; // ����һ����ʽ�ṹ��ѭ�������ǻ���
	}
	_CCSpanList[index].CC_mtx.unlock();

}