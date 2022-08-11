#define _CRT_SECURE_NO_WARNINGS 1
#include "PageCache.h"

PageCache PageCache::_PC;

void PageCache::ReleaseSpanToPC(Span* span) // �ϲ��ڴ�
{
	if (span->_n > MAXPAGES - 1)
	{
		// ֱ�ӻ�����
		SystemFree((void*)(span->_pageId << PAGESHIFT));
		//delete span;
		_SpanPool.Delete(span);

		return;
	}

	while (1)
	{
		// ����ǰ�����ڵ�ҳ��β
		
		// û��ҳ�ţ���Ҫ�ϲ��� -- �����汾
		PAGE_ID previd = span->_pageId - 1;
		auto ret = (Span*)_IdSpanMap.get(previd);
		if (ret == nullptr)
		{
			break; // û�о�break
		}
		//auto ret = _IdSpanMap.find(previd);
		//if (ret == _IdSpanMap.end())
		//{
		//	break; // û�о�break
		//}

		Span* prevSpan = ret;
		if (prevSpan->_IsUse) // == true
		{
			break; // ��ʹ��
		}

		if (prevSpan->_n + span->_n > MAXPAGES - 1)
		{
			break; // ҳ������128
		}
		
		// ��ǰ�ϲ�
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_PCSpanList[span->_n].Erase(prevSpan);
		//delete prevSpan;
		_SpanPool.Delete(prevSpan);

	}

	// ���ϲ�
	while (1)
	{
		// ���Һ������ڵ�ҳ��ͷ
		//PAGE_ID nextid = span->_pageId + span->_n;
		//auto ret = _IdSpanMap.find(nextid);
		//if (ret == _IdSpanMap.end())
		//{
		//	break; // û�о�break
		//}

		PAGE_ID previd = span->_pageId - 1;
		auto ret = (Span*)_IdSpanMap.get(previd);
		if (ret == nullptr)
		{
			break; // û�о�break
		}
		Span* nextSpan = ret;
		if (nextSpan->_IsUse)
		{
			break; // ��ʹ��
		}

		if (nextSpan->_n + span->_n > MAXPAGES - 1)
		{
			break; // ҳ������128
		}

		// ���ϲ�
		span->_n += nextSpan->_n;

		_PCSpanList[span->_n].Erase(nextSpan);
		// delete nextSpan;
		_SpanPool.Delete(nextSpan);
	}
	// �ϲ�������
	_PCSpanList[span->_n].PushFront(span);
	span->_IsUse = false;
	//_IdSpanMap[span->_pageId] = span;
	//_IdSpanMap[span->_pageId + span->_n - 1] = span;

	_IdSpanMap.set(span->_pageId, span);
	_IdSpanMap.set(span->_pageId + span->_n - 1, span);

}

Span* PageCache::MapObjectToSpan(void* obj) // ��ȡ�Ӷ���span�Ķ�Ӧ��ϵ
{
	PAGE_ID id = (PAGE_ID)obj >> PAGESHIFT; // ��8k��ҪתΪ10����

	Span* span = (Span*)_IdSpanMap.get(id);

	//std::unique_lock<std::mutex> lock(PC_mtx); // ����ҲҪ����

	//auto ret = _IdSpanMap.find(id);
	//if (ret != _IdSpanMap.end())
	//{
	//	return ret->second; // �ҵ����� 
	//}
	//else
	//{
	//	assert(false);
	//	return nullptr;
	//	// �������Ҳ���;
	//}
	return span;
}

Span* PageCache::NewSpan(size_t page) // ������Ӧ��span, k��ҳ
{
	assert(page > 0);
	
	if (page > MAXPAGES-1)
	{
		// ������
		void* ptr = SystemAlloc(page);
		Span* span = _SpanPool.New();

		span->_pageId = (PAGE_ID)ptr >> PAGESHIFT;
		span->_n = page;

		//_IdSpanMap[span->_pageId] = span;
		_IdSpanMap.set(span->_pageId, span);

		return span;
	}

	// �ȿ���Ͱ������span���о͸�һ��page������
	if (!_PCSpanList[page].Empty())
	{
		Span* kspan = _PCSpanList[page].PopFront();
		// �õľͽ���ÿ��ӳ��
		for (PAGE_ID i = 0; i < kspan->_n; i++)
		{
			//_IdSpanMap[kspan->_pageId + i] = kspan;
			_IdSpanMap.set(kspan->_pageId + i, kspan);

		}
		return kspan;
	}

	// û�оͱ��������Ͱ,��ֱ����û��
	for (int n = page + 1; n < MAXPAGES; n++)
	{
		if (!_PCSpanList[n].Empty()) // ˵���У����з֣�����Ҫ2��3 �о�Ҫ�г� 1 �� 2
		{
			Span* nspan = _PCSpanList[n].PopFront(); // ��������,ʣ����ٹ���
			Span* kspan = _SpanPool.New(); // k = n - page������Ҫ��CC��

			// ��nspan��ͷ��һ������
			kspan->_pageId = nspan->_pageId;
			kspan->_n = page;
			nspan->_pageId += page; // ������
			nspan->_n -= page;

			_PCSpanList[nspan->_n].PushFront(nspan);

			// ��ʱ���õľͣ���ͷβ���ӳ��
			//_IdSpanMap[nspan->_pageId] = nspan;
			//_IdSpanMap[nspan->_pageId + nspan->_n - 1] = nspan; // ����һ��ӳ��Խ����

			_IdSpanMap.set(nspan->_pageId, nspan);
			_IdSpanMap.set(nspan->_pageId + nspan->_n - 1, nspan);


			// �õľͽ���ÿ��ӳ��
			for (PAGE_ID i = 0; i < kspan->_n; i++)
			{
				//_IdSpanMap[kspan->_pageId + i] = kspan;
				_IdSpanMap.set(kspan->_pageId + i, kspan);
			}

			return kspan;
		}
	}

	// û�и����ҳ�ˣ����������ռ�

	Span* BigSpan = _SpanPool.New();
	void* ptr = SystemAlloc(MAXPAGES - 1); // ���� 128 ��ҳ  128 * 8k �Ĵ���ڴ�

	BigSpan->_pageId = (PAGE_ID)ptr >> PAGESHIFT; // ��8k����ҳ�ţ����忴����
	BigSpan->_n = MAXPAGES - 1; // ����ڴ��ҳ����

	_PCSpanList[BigSpan->_n].PushFront(BigSpan); // ���뵽PC��������

	return NewSpan(page); // �ݹ����һ���Լ������з�
}