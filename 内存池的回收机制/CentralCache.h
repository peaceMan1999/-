#pragma once
#include "Common.h"

// ����ģʽ--����ģʽ������ȫ��Ψһʵ��������--���Ļ���
class CentralCache
{
public:
	static CentralCache* GetInstance() // ��ȡʵ��������
	{
		return &_cc;
	}

	Span* GetOneSpan(SpanList& list, size_t bytes); // ����SpanΪ�գ���ȡSpan

	size_t GetRangeObj(void*& start, void*& end, size_t batchNum, size_t bytes); // �����Ļ�ȡһ�������Ķ����TC

	void ReleaseListToSpan(void* start, size_t bytes); // �����TC���յ���������

private:
	CentralCache()
	{};

	CentralCache(const CentralCache& p) = delete;

	static CentralCache _cc; // ����

	SpanList _CCSpanList[MAXBUCKETS]; // CC�Ĺ�ϣ��
};