#include "memoryblock.h"
#include "cnrt.h"
#include <iostream>

std::mutex MLUMemoryBlock::sMutex;
std::atomic<MLUMemoryBlock *> MLUMemoryBlock::sInstance(nullptr);

MLUMemoryBlock::MLUMemoryBlock()
{

}

MLUMemoryBlock::~MLUMemoryBlock()
{
    if (mStartPtr != nullptr) {
        cnrtFree((void*)(mStartPtr));
    }
}

bool MLUMemoryBlock::Init(size_t size)
{
    if (mStartPtr != nullptr || !mItemList.empty()) {
        std::cout << "MLU memory init fail" << std::endl;
        return false;
    }
    mCapacity = size;
    int cnt = 0;
    //开辟内存retry五次
    while (true) {
        int iRet = cnrtMalloc((void **)&mStartPtr, mCapacity);
        if (iRet == cnrtSuccess) {
            break;
        } else {
            std::cout << "MLU init error retcode" << iRet << std::endl;
        }
        if (cnt == 5) {
            std::cout << "MLU memory init fail, retry 5 times" << std::endl;
            return false;
        }
        cnt++;
    }
    MemoryItem item(mStartPtr, mCapacity, 0);
    mEndPtr = (int8_t *)mStartPtr + mCapacity;
    mItemList.push_back(item);
    return true;
}

void* MLUMemoryBlock::Allocate(size_t size)
{
    if (size <= 0 || size > mCapacity) {
        std::cout << "Alloc size is invalid" << std::endl;
        return nullptr;
    }

    size_t len = GetAlignedSize(size);
    void *tmpPtr = GetFitItem(len);
    if (tmpPtr != nullptr) {
        mUsedLength += len;
        return tmpPtr;
    }
    return nullptr;
}

void *MLUMemoryBlock::GetFitItem(size_t len)
{
    for (auto it = mItemList.begin(); it != mItemList.end(); it++) {
        if ((*it).IsUsed()) {continue;}

        if ((*it).length_ == len) {
            (*it).ref_count_ = 1;
            return (*it).ptr_;
        } else if ((*it).length_ > len) {
            MemoryItem tmp((*it).ptr_, len, 1);

            (*it).ptr_ = (void *)((int8_t *)(*it).ptr_ + len);
            (*it).length_ = (*it).length_ - len;
            (*it).ref_count_ = 0;

            mItemList.insert(it, tmp);
            return tmp.ptr_;
        }
    }
    printf("Get fit item for size = %zd fail, item list size %zd", len, mItemList.size());
    return nullptr;
}

bool MLUMemoryBlock::InBlock(void *ptr) 
{
    if (!ptr) {
        return false;
    }
    if (ptr < mStartPtr || ptr > mEndPtr) {
        return false;
    }
    return true;
}

void *MLUMemoryBlock::CopyPtr(void *ptr)
{
    if (!ptr) {
        std::cout << "Ptr is NULL" << std::endl;
    }
    for (auto it = mItemList.begin(); it != mItemList.end(); it++) {
        if ((*it).ptr_ != ptr) {continue;}
        //item指针未被使用，返回失败
        if (!(*it).IsUsed()) {
            std::cout << "ptr=" << ptr << " is already freed" << std::endl;
            return nullptr;
        }

        (*it).ref_count_++;
        return ptr;
    }
    std::cout << "ptr=" << ptr << " is not allocated" << std::endl;
    return nullptr;
}

bool MLUMemoryBlock::Free(void *ptr)
{
    if (!ptr) {
        std::cout << "ptr is nullptr" << std::endl;
        return false;
    }
    for (auto it = mItemList.begin(); it != mItemList.end(); it++) {
        if ((*it).ptr_ != ptr) {continue;}
        //item指针未被使用，返回失败
        if (!(*it).IsUsed()) {
            std::cout << "ptr=" << ptr << " is already freed" << std::endl;
            return false;
        }

        (*it).ref_count_--;
        //被多个user使用，引用计数减一返回
        if ((*it).IsUsed()) {
            return true;
        }
        //重新计算usedLength
        mUsedLength -= (*it).length_;

        Mergeitem(it);
        return true;
    }
    std::cout << "ptr=" << ptr << " is not allocated" << std::endl;
    return false;
}

void MLUMemoryBlock::Mergeitem(std::list<MemoryItem>::iterator &it)
{
    //跟上一个进行合并
    if (it != mItemList.begin()) {
        auto pre = it;
        auto current = it;
        pre --;
        if ((*pre).IsUsed() == false) {
            //合并
            (*current).length_ = (*current).length_ + (*pre).length_;
            (*current).ptr_ = (*pre).ptr_;
            (*current).ref_count_ = 0;
            //删除前一个iterator，返回当前的iterator
            it = mItemList.erase(pre);
        }
    }

    if (it != mItemList.end()) {
        auto current = it;
        auto next = it;
        next++;
        if ((*next).IsUsed() == false) {
            //合并到后一个iterator
            (*next).length_ = (*current).length_ + (*next).length_;
            (*next).ptr_ = (*current).ptr_;
            (*next).ref_count_ = 0;
            //删除当前iterator
            it = mItemList.erase(current);
        }
    }
    return;
}

size_t MLUMemoryBlock::GetFreeLength() const 
{
    return mCapacity - mUsedLength;
}

size_t MLUMemoryBlock::GetUsedLength() const
{
    return mUsedLength;
}

size_t MLUMemoryBlock::GetCapacity() const
{
    return mCapacity;
}

size_t MLUMemoryBlock::GetAlignedSize(size_t size)
{
    return (size + MLU_MEMORY_PITCH_SIZE - 1)/MLU_MEMORY_PITCH_SIZE * MLU_MEMORY_PITCH_SIZE;
}

size_t MLUMemoryBlock::GetItemCount() const
{
    return mItemList.size();
}

MLUMemoryBlock *MLUMemoryBlock::GetInstance()
{
    MLUMemoryBlock *mluMemoryBlock = sInstance.load();
    if (mluMemoryBlock) { return mluMemoryBlock; }

    std::lock_guard<std::mutex> lock(sMutex);
    MLUMemoryBlock *mluMemoryBlock_1 = sInstance.load();
    if (mluMemoryBlock_1) { return mluMemoryBlock_1; }

    MLUMemoryBlock *mluMemoryBlock_2 = new MLUMemoryBlock();
    mluMemoryBlock_2->Init(1024 * 1024 * 1024);
    if (mluMemoryBlock_2 == nullptr) return nullptr;
    std::unique_ptr<MLUMemoryBlock> mluMemoryBlock_new(mluMemoryBlock_2);
    sInstance.store(mluMemoryBlock_new.release());

    return sInstance.load();
}

void MLUMemoryBlock::DestoryInstance()
{
    std::lock_guard<std::mutex> lock(sMutex);
    MLUMemoryBlock* mluMemoryBlock = sInstance.load();
    if (mluMemoryBlock) { delete mluMemoryBlock; }
}
