#ifndef CNPIPE_CN_MEMORY_POOL_H
#define CNPIPE_CN_MEMORY_POOL_H

#include <climits>
#include <cstddef>
#include <atomic>
#include <type_traits>
#include <utility>
#include <chrono>
#include <iostream>
#include <thread>
#include <cassert>
#include <mutex>
#include <cnrt.h>

class MemoryPool
{
private:
    const size_t BlockSize = 4096;
    mutable std::mutex mLock;
public:
    /* Member functions */
    MemoryPool() noexcept;
    MemoryPool(const MemoryPool& memoryPool) noexcept;
    MemoryPool(MemoryPool&& memoryPool) noexcept;

    ~MemoryPool() noexcept;

    MemoryPool& operator=(const MemoryPool& memoryPool) = delete;
    MemoryPool& operator=(MemoryPool&& memoryPool) noexcept;

    // Can only allocate one object at a time. n and hint are ignored
    void* allocate(size_t n = 1);
    void deallocate(void* p, size_t n = 1);

    size_t max_size() const noexcept;

  private:
    union Slot_ {
      void* element;
      Slot_* next;
    };

    Slot_* currentBlock_;
    Slot_* currentSlot_;
    Slot_* lastSlot_;
    Slot_* freeSlots_;

    size_t padPointer(void* p, size_t align) const noexcept;
    void allocateBlock();
};

size_t MemoryPool::padPointer(void* p, size_t align)
const noexcept
{
    uintptr_t result = reinterpret_cast<uintptr_t>(p);
    return ((align - result) % align);
}

MemoryPool::MemoryPool()
noexcept
{
  currentBlock_ = nullptr;
  currentSlot_ = nullptr;
  lastSlot_ = nullptr;
  freeSlots_ = nullptr;
}

MemoryPool::MemoryPool(const MemoryPool& memoryPool)
noexcept :
MemoryPool()
{}

MemoryPool::MemoryPool(MemoryPool&& memoryPool)
noexcept
{
  currentBlock_ = memoryPool.currentBlock_;
  memoryPool.currentBlock_ = nullptr;
  currentSlot_ = memoryPool.currentSlot_;
  lastSlot_ = memoryPool.lastSlot_;
  freeSlots_ = memoryPool.freeSlots_;
}

MemoryPool& MemoryPool::operator=(MemoryPool&& memoryPool)
noexcept
{
    if (this != &memoryPool)
    {
        std::swap(currentBlock_, memoryPool.currentBlock_);
        currentSlot_ = memoryPool.currentSlot_;
        lastSlot_ = memoryPool.lastSlot_;
        freeSlots_ = memoryPool.freeSlots_;
    }
    return *this;
}

MemoryPool::~MemoryPool()
noexcept
{
  Slot_* curr = currentBlock_;
  while (curr != nullptr) {
    Slot_* prev = curr->next;
    operator delete(reinterpret_cast<void*>(curr));
    curr = prev;
  }
}

void MemoryPool::allocateBlock()
{
    Slot_* newBlock = operator new();
    CNPEAK_CHECK_CNRT(cnrtMalloc(&newBlock, BlockSize * 1024 * 1024));
  // Allocate space for the new block and store a pointer to the previous one
  void* newBlock = reinterpret_cast<data_pointer_>
                           (operator new(BlockSize));
  reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
  currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);
  // Pad block body to staisfy the alignment requirements for elements
  data_pointer_ body = newBlock + sizeof(slot_pointer_);
  size_type bodyPadding = padPointer(body, alignof(slot_type_));
  currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
  lastSlot_ = reinterpret_cast<slot_pointer_>
              (newBlock + BlockSize - sizeof(slot_type_) + 1);
}



template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_t n)
{
  if (freeSlots_ != nullptr) {
    pointer result = reinterpret_cast<pointer>(freeSlots_);
    freeSlots_ = freeSlots_->next;
    return result;
  }
  else {
    if (currentSlot_ >= lastSlot_)
      allocateBlock();
    return reinterpret_cast<pointer>(currentSlot_++);
  }
}



template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n)
{
  if (p != nullptr) {
    reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
    freeSlots_ = reinterpret_cast<slot_pointer_>(p);
  }
}



template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::max_size()
const noexcept
{
  size_type maxBlocks = -1 / BlockSize;
  return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_type_) * maxBlocks;
}



template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void
MemoryPool<T, BlockSize>::construct(U* p, Args&&... args)
{
  new (p) U (std::forward<Args>(args)...);
}



template <typename T, size_t BlockSize>
template <class U>
inline void
MemoryPool<T, BlockSize>::destroy(U* p)
{
  p->~U();
}



template <typename T, size_t BlockSize>
template <class... Args>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(Args&&... args)
{
  pointer result = allocate();
  construct<value_type>(result, std::forward<Args>(args)...);
  return result;
}



template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deleteElement(pointer p)
{
  if (p != nullptr) {
    p->~value_type();
    deallocate(p);
  }
}


#endif // CNPIPE_CN_MEMORY_POOL_H