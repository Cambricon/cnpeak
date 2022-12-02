#ifndef CNPIPE_MEMORYBLOCK_H
#define CNPIPE_MEMORYBLOCK_H


#include <string>
#include <memory>
#include <list>
#include <atomic>
#include <mutex>

struct MemoryItem
{
    void        *ptr_;
    size_t      length_ = 0;
    int32_t     ref_count_ = 0;

    MemoryItem(void *ptr = nullptr, size_t len = 0, int32_t refCount = 0) {
        ptr_ = ptr;
        length_ = len;
        ref_count_ = refCount;
    }
    bool IsUsed() const
    {
        return ref_count_ > 0;
    }
};

class MLUMemoryBlock
{
public:
    MLUMemoryBlock();
    ~MLUMemoryBlock();
private:
    MLUMemoryBlock(const MLUMemoryBlock &);
    MLUMemoryBlock& operator=(const MLUMemoryBlock &);
public:
    static MLUMemoryBlock *GetInstance();
    static void DestoryInstance();
public:
    // 申请多大的显存，里面retry
    bool Init(size_t size);

    void *Allocate(size_t size);
    bool InBlock(void *ptr);
    bool Free(void *ptr);
    void *CopyPtr(void *ptr);

    size_t GetFreeLength() const;
    size_t GetUsedLength() const;
    size_t GetCapacity() const;

    static size_t GetAlignedSize(size_t size);
    size_t GetItemCount() const;
private:
    void *GetFitItem(size_t len);
    void Mergeitem(std::list<MemoryItem>::iterator &it);
private:
    size_t mCapacity = 0;
    void *mStartPtr = nullptr;
    void *mEndPtr = nullptr;

    size_t mUsedLength = 0;
    std::list<MemoryItem> mItemList;

    const static size_t MLU_MEMORY_PITCH_SIZE = 2;
private:
    static std::atomic<MLUMemoryBlock*> sInstance;
    static std::mutex sMutex;
};

#endif