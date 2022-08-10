#pragma once
#include "Common.h"

template<class T>
class ObjectPool
{
public:
    T* New()
    {
        T* obj = nullptr;
        if (_freeList) //�黹���ڴ����
        {
            void* next = *((void**)_freeList); // ͷɾ����ָ��
            obj = (T*)_freeList;
            _freeList = next;
        }
        else {
            if (_mBytesSize < sizeof(T)) // �п��ܲ�������������Ҫ��T�Ƚϴ�С
            {
                _mBytesSize = 128 * 1024;
                //_memery = (char*)malloc(_mBytesSize);
                _memery = (char*)SystemAlloc(_mBytesSize >> PAGESHIFT);
                if (_memery == nullptr) {
                    throw std::bad_alloc();
                }
            }
            obj = (T*)_memery; // Ԥ��ָ����ʣ��T����,�޷�����ָ��

            size_t Size = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
            _memery += Size; // ������
            _mBytesSize -= Size; // ���˶��ټ�ȥ����
        }

        new(obj)T; // ��λnew����ʾ���캯����ʼ��

        return obj;
    }

    void Delete(T* obj)
    {
        obj->~T(); // �ڿ�ͷҪ��ʾ�������������������
        //ͷ��
        *(void**)obj = _freeList; // Ϊʲô��void**����Ϊ��ָ���С�����ݲ���ϵͳ�仯
        _freeList = obj;
    }

private:
    char* _memery = nullptr; // ���ٵĿռ�
    void* _freeList = nullptr; // ���ؿռ������
    size_t _mBytesSize = 0; // �ռ��е�ʣ���ֽڴ�С��
};


struct TreeNode
{
    int _val;
    TreeNode* _left;
    TreeNode* _right;

    TreeNode()
        :_val(0)
        , _left(nullptr)
        , _right(nullptr)
    {}
};
//
//void TestObjectPool()
//{
//    // �����ͷŵ��ִ�
//    const size_t Rounds = 5;
//
//    // ÿ�������ͷŶ��ٴ�
//    const size_t N = 100000;
//
//    std::vector<TreeNode*> v1;
//    v1.reserve(N);
//
//    size_t begin1 = clock();
//
//    for (size_t j = 0; j < Rounds; ++j)
//    {
//        for (int i = 0; i < N; ++i)
//        {
//            v1.push_back(new TreeNode);
//        }
//        for (int i = 0; i < N; ++i)
//        {
//            delete v1[i];
//        }
//        v1.clear();
//    }
//
//    size_t end1 = clock();
//
//    std::vector<TreeNode*> v2;
//    v2.reserve(N);
//
//    ObjectPool<TreeNode> TNPool;
//    size_t begin2 = clock();
//    for (size_t j = 0; j < Rounds; ++j)
//    {
//        for (int i = 0; i < N; ++i)
//        {
//            v2.push_back(TNPool.New());
//        }
//        for (int i = 0; i < N; ++i)
//        {
//            TNPool.Delete(v2[i]);
//        }
//        v2.clear();
//    }
//    size_t end2 = clock();
//
//    cout << "new cost time:" << end1 - begin1 << endl;
//    cout << "object pool cost time:" << end2 - begin2 << endl;
//}

