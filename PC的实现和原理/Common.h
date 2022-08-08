#pragma once
#include <iostream>
#include <vector>
#include <time.h>
#include <assert.h>
#include <thread>
#include <algorithm>
#include <mutex>

using std::cout;
using std::endl;

// �������ú�
static const size_t MAXBYTES = 258 * 1024;  // ���bytes
static const size_t MAXBUCKETS = 208;		// ���Ͱ��
static const size_t MAXPAGES = 129;			// ���ҳ��
static const size_t PAGESHIFT = 13;			// �������8k



// 64Ҫ��ǰ�棬��Ϊx64�¼���32����64
#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#else
	// linux
#endif

#ifdef _WIN64
	#include <windows.h>	
#elif _WIN32
	#include <windows.h>	
#else
	// linux
#endif

// ֱ��ȥ���ϰ�ҳ����ռ�
inline static void* SystemAlloc(size_t kpage) // ҳ
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// linux��brk mmap��
#endif

	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

// ���徲̬����
static void*& NextObj(void* obj)
{
	return *(void**)obj; // ǰ4/8λ
}

// ����һ���࣬�����������зֺõĶ������������
class FreeList
{
public:
	void Push(void* obj) // ����ͷ��
	{
		assert(obj);

		NextObj(obj) = _freeList; // �����ڴ�ؽ���
		_freeList = obj;
	}

	void PushRange(void* start, void* end)
	{
		NextObj(end) = _freeList;
		_freeList = start;
	}

	void* Pop() // �����ڴ棬ͷɾ
	{
		assert(_freeList);

		void* obj = _freeList;
		_freeList = NextObj(obj);
		return obj;
	}

	size_t& MaxSize()
	{
		return maxSize;
	}

	bool Empty() // �п�
	{
		return _freeList == nullptr;
	}
private:
	void* _freeList = nullptr; // ��һ��ָ����Ϊͷ�ڵ�
	size_t maxSize = 1;
};


// ����һ���࣬������������ֽں�Ͱ
class SizeClass
{
public:
	// Ϊʲô�þ�̬��������Ϊ��Ա������Ҫ��һ����ȥ���ã���SizeClass��û�г�Ա���־������ã�Ϊ�˷���
	static inline size_t _RoundUp(size_t bytes, size_t alignByte)
	{
		return ((bytes + alignByte - 1) & ~(alignByte - 1)); // ���� 8 + 7 = 15 &~ 7 = 8;���忴���ͣ�
	}

	static inline size_t RoundUp(size_t bytes) // ����ʵ�ʶ����ı���λ��С
	{
		// ������������10%���ҵ�����Ƭ�˷ѣ�����ԭ���ҵĲ��ͣ�
		// [1,128]					8byte����			freelist[0,16)
		// [128+1,1024]				16byte����			freelist[16,72)
		// [1024+1,8*1024]			128byte����			freelist[72,128)
		// [8*1024+1,64*1024]		1024byte����		freelist[128,184)
		// [64*1024+1,256*1024]		8*1024byte����		freelist[184,208)
		assert(bytes <= MAXBYTES); // �治ִ�У���ִ��
		if (bytes <= 128) { // С��128����8�ֽڶ���
			return _RoundUp(bytes, 8);
		}
		else if (bytes <= 1024) {
			return _RoundUp(bytes, 16);
		}
		else if (bytes <= 8 * 1024) {
			return _RoundUp(bytes, 128);
		}
		else if (bytes <= 64 * 1024) {
			return _RoundUp(bytes, 1024);
		}
		else if (bytes <= 256 * 1024) {
			return _RoundUp(bytes, 8 * 1024);
		}
		else { // �ж�ʧ�ܣ�������
			cout << "RoundUp err" << endl;
			return -1;
		}
	}

	static inline size_t _Index(size_t bytes, size_t alignShift)
	{
		return (((bytes + (1 << alignShift) -1) >> alignShift)-1);
		// ����  1 << 3 = 8; 1~8 + 7 = 8~15; 8 >> 3 = 1��15 >> 3 = 1; -1 = 0; ����1~8��0��Ͱ
	}

	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAXBYTES);
		// ���������ǿ��Կ���ÿ���׶�Ͱ��λ�ú�������಻����208��
		static size_t bucketArray[4] = { 16, 72, 128, 184 };
		if (bytes <= 128) { // 128��Χ��Ӧ����0~15��Ͱ��һ��16��
			return _Index(bytes, 3); // alignShift����1�����ƴ���/2�Ĵη�
		}
		else if (bytes <= 1024) {
			return _Index(bytes, 4) + bucketArray[0]; // Ҫ����ǰ���Ͱ�����忴����
		}
		else if (bytes <= 8 * 1024) {
			return _Index(bytes, 7) + bucketArray[1];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes, 10) + bucketArray[2];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes, 13) + bucketArray[3];
		}
		else { // �ж�ʧ�ܣ�������
			cout << "Index err" << endl;
			return -2;
		}
	}

	static size_t NumMoveSize(size_t bytes) // һ����ȥ���ٶ���
	{
		assert(bytes <= MAXBYTES);
		assert(bytes > 0);
		// [2, 512] ����õķ�Χ�����忴����
		size_t num = MAXBYTES / bytes;

		if (num < 2) // ��С����
		{
			num = 2;
		}
		if (num > 512) // ������
		{
			num = 512;
		}

		return num;
	}

	// ����һ����ϵͳ��ȡ����ҳ
	// �������� 8byte
	// ...
	// �������� 256KB
	static size_t NumMovePage(size_t bytes)
	{
		size_t num = NumMoveSize(bytes);
		size_t npage = num * bytes;

		npage >>= PAGESHIFT;
		if (npage == 0) // һ���������͸�һ��
			npage = 1;

		return npage;
	}
};

struct Span
{
	PAGE_ID _pageId = 0; // ҳ��
	size_t _n = 0; // ҳ������ҳ�ı���������PC�е�ÿһ���±�
	Span* _next = nullptr; // ˫����
	Span* _prev = nullptr;
	size_t _usecount = 0; // ���ڹ黹�ڴ�
	void* _SpanFreeList = nullptr; // �кõ���������
};

class SpanList // ��������ÿһ��bytes�����Span����
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End() // ��Ϊ��˫����������end��ͷ�ڵ�
	{
		return _head;
	}

	void PushFront(Span* obj)
	{
		Insert(Begin(), obj);
	}

	Span* PopFront() // ��span����ȥ��βɾ
	{
		Span* obj = _head->_next;
		Erase(obj);
		return obj;
	}

	bool Empty()
	{
		return _head == _head->_next;
	}

	void Insert(Span* pos, Span* obj) // ͷ��
	{
		assert(pos);
		assert(obj);
		Span* prev = pos->_prev;
		prev->_next = obj;
		obj->_prev = prev;

		obj->_next = pos;
		pos->_prev = obj;
	}

	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos != _head); // ���ͷ�ڵ�ɾ��
		pos->_prev->_next = pos->_next;
		pos->_next->_prev = pos->_prev;
	}

private:
	Span* _head; // ͷ�ڵ�
public:
	std::mutex CC_mtx; // Ͱ��
};
