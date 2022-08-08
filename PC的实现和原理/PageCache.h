#pragma once
#include "Common.h"


// Ҳ��һ������ģʽ��ȫ��Ψһ
class PageCache
{
public:
	static PageCache* GetInstance() // ��ȡʵ��������
	{
		return &_PC;
	}

	Span* NewSpan(size_t page); // ������Ӧ��span

	std::mutex PC_mtx; // ����PCһ����������

private:
	PageCache()
	{};

	PageCache(const PageCache& p) = delete;

	static PageCache _PC;

	SpanList _PCSpanList[MAXPAGES];

};






