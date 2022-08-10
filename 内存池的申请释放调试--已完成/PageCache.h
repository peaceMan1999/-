#pragma once
#include "Common.h"
#include "ObjectPool.h"


// Ҳ��һ������ģʽ��ȫ��Ψһ
class PageCache
{
public:
	static PageCache* GetInstance() // ��ȡʵ��������
	{
		return &_PC;
	}

	Span* NewSpan(size_t page); // ������Ӧ��span

	Span* MapObjectToSpan(void* obj); // ��ȡ�Ӷ���span�Ķ�Ӧ��ϵ

	void ReleaseSpanToPC(Span* span); // �ϲ��ڴ�

	std::mutex PC_mtx; // ����PCһ����������

private:
	PageCache()
	{};

	ObjectPool<Span> _SpanPool;

	PageCache(const PageCache& p) = delete;

	static PageCache _PC;
	
	std::unordered_map<PAGE_ID, Span*> _IdSpanMap; // ӳ���ϵ

	SpanList _PCSpanList[MAXPAGES];

};






