#define _CRT_SECURE_NO_WARNINGS 1
#include "PageCache.h"

PageCache PageCache::_PC;

Span* PageCache::NewSpan(size_t page) // ������Ӧ��span, k��ҳ
{
	assert(page > 0 && page < MAXPAGES);

	// �ȿ���Ͱ������span���о͸�һ��page������
	if (!_PCSpanList[page].Empty())
	{
		return _PCSpanList[page].PopFront();
	}

	// û�оͱ��������Ͱ,��ֱ����û��
	for (int n = page + 1; n < MAXPAGES; n++)
	{
		if (!_PCSpanList[n].Empty()) // ˵���У����з֣�����Ҫ2��3 �о�Ҫ�г� 1 �� 2
		{
			Span* nspan = _PCSpanList[n].PopFront(); // ��������,ʣ����ٹ���
			Span* kspan = new Span; // k = n - page������Ҫ��CC��

			// ��nspan��ͷ��һ������
			kspan->_pageId = nspan->_pageId;
			kspan->_n = page;
			nspan->_pageId += page; // ������
			nspan->_n -= page;

			_PCSpanList[nspan->_n].PushFront(nspan);

			return kspan;
		}
	}

	// û�и����ҳ�ˣ����������ռ�

	Span* BigSpan = new Span;
	void* ptr = SystemAlloc(MAXPAGES - 1); // ���� 128 ��ҳ  128 * 8k �Ĵ���ڴ�

	BigSpan->_pageId = (PAGE_ID)ptr >> PAGESHIFT; // ��8k����ҳ�ţ����忴����
	BigSpan->_n = MAXPAGES - 1; // ����ڴ��ҳ����

	_PCSpanList[BigSpan->_n].PushFront(BigSpan); // ���뵽PC��������

	return NewSpan(page); // �ݹ����һ���Լ������з�
}