#include <cnrt.h>
#include <string.h>

#include "tools.hpp"
#include "cnData.h"
#include "memoryblock.h"

class cnData::_Data {
public:
    mutable void *mCPUPtr = nullptr;
    mutable void *mMLUPtr = nullptr;
    size_t mLength = 0;
    MLUMemoryBlock *ctx;
};

cnData::cnData() {
    mData = new _Data();
    mData->ctx = MLUMemoryBlock::GetInstance();
}

cnData::~cnData() {
    Release();
    delete mData;
}

bool cnData::Malloc(size_t size) {
    if (!Release()) {
        return false;
    }
    if (!mData->ctx) {
        return false;
    }
    mData->mMLUPtr = mData->ctx->Allocate(size);
    if (!mData->mMLUPtr) {
        return false;
    }
    mData->mLength = size;
    return true;
}

bool cnData::Release()
{
    if (!mData->mCPUPtr) {
        free(mData->mCPUPtr);
        mData->mCPUPtr = nullptr;
    }
    if (!mData->mMLUPtr) {
        if (!mData->ctx) {
            return false;
        }
        mData->ctx->Free(mData->mMLUPtr);
        mData->mMLUPtr = nullptr;
    }
    mData->mLength = 0;
    return true;
}

bool cnData::MLUToCPU(const void *src, void *&dst, size_t len) const
{
    if (src == nullptr) { return true; }
    CNPEAK_CHECK_CNRT(cnrtMemcpy(reinterpret_cast<void*>(dst), const_cast<void*>(src), len, CNRT_MEM_TRANS_DIR_DEV2HOST));
    return true;
}

bool cnData::MLUToMLU(const void *src, void *&dst, size_t len) const
{
    if (src == nullptr) { return true; }
    if (!mData->ctx) {
        return false;
    }
    if (dst) { mData->ctx->Free(dst); }
    dst = mData->ctx->Allocate(len);
    if (!dst) {
        return false;
    }
    CNPEAK_CHECK_CNRT(cnrtMemcpy(dst, const_cast<void *>(src), len, CNRT_MEM_TRANS_DIR_DEV2DEV));
    return true;
}

bool cnData::CPUToMLU(const void *src, void *&dst, size_t len) const
{
    if (src == nullptr) { return true; }
    if (!mData->ctx) {
        return false;
    }
    if (dst) { mData->ctx->Free(dst); }
    dst = mData->ctx->Allocate(len);
    if (!dst) {
        return false;
    }
    CNPEAK_CHECK_CNRT(cnrtMemcpy(dst, const_cast<void *>(src), len, CNRT_MEM_TRANS_DIR_HOST2DEV));
    return true;
}

void *cnData::GetMLUPtr() const
{
    if (mData->mMLUPtr) { return mData->mMLUPtr; }
    if (!mData->mLength) { return nullptr; }
    if (!CPUToMLU(mData->mCPUPtr, mData->mMLUPtr, mData->mLength)) {
        return nullptr;
    }
    return mData->mMLUPtr;
}

size_t cnData::GetLength() const
{
    return mData->mLength;
}

bool cnData::SetToCPU(void *ptr, int len)
{
    if (!ptr || len == 0) {
        return false;
    }
    if (!Release()) {
        return false;
    }
    mData->mCPUPtr = (void*)malloc(len);
    memcpy(mData->mCPUPtr, ptr, len);
    mData->mLength = len;
    return true;
}

void *cnData::StealMLUPtr(size_t &len)
{
    len = mData->mLength;
    if (nullptr == mData->mMLUPtr) {
        if (!CPUToMLU(mData->mCPUPtr, mData->mMLUPtr, mData->mLength)) {
            return nullptr;
        }
    }
    void *tmpPtr = mData->mMLUPtr;
    mData->mMLUPtr = nullptr;
    if (!Release()) {
        return nullptr;
    }
    return tmpPtr;
}

void *cnData::StealCPUPtr(size_t &len) {
    len = mData->mLength;
    if (nullptr == mData->mCPUPtr) {
        if (!MLUToCPU(mData->mMLUPtr, mData->mCPUPtr, mData->mLength)) {
            return nullptr;
        }
    }
    void *tmpPtr = mData->mCPUPtr;
    mData->mCPUPtr = nullptr;
    if (!Release()) {
        return nullptr;
    }
    return tmpPtr;
}

bool cnData::SetToMLU(void *ptr, int len)
{
    if (!ptr || len == 0) {
        return false;
    }
    if (!Release()) {
        return false;
    }
    mData->mMLUPtr = ptr;
    mData->mLength = len;
    return true;
}

cnData* cnData::Clone() const
{
    cnData *data = new cnData();
    auto ctx = mData->ctx;
    if (ctx == nullptr) {
        if (!MLUToMLU(mData->mMLUPtr, data->mData->mMLUPtr, mData->mLength)) {
            return nullptr;
        }
    } else {
        data->mData->mMLUPtr = ctx->CopyPtr(mData->mMLUPtr);
    }
    data->mData->mCPUPtr = (void*)malloc(mData->mLength);
    memcpy(data->mData->mCPUPtr, mData->mCPUPtr, mData->mLength);
    data->mData->mLength = mData->mLength;
    return data;
}