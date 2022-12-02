#include "cnStruct.h"

cnStruct::cnStruct()
{
}

cnStruct::~cnStruct()
{
    Release();
}

bool cnStruct::GetStringValue(const std::string &fieldName, std::string& value) const
{
    std::lock_guard<std::mutex> lck(mLock);
    auto it = mValues.find(fieldName);
    if (it == mValues.end()) {
        return false;
    }
    value = it->second;
    return true;
}

bool cnStruct::SetStringValue(const std::string &fieldName, const std::string &value)
{
    std::lock_guard<std::mutex> lck(mLock);
    mValues[fieldName] = value;
    return true;
}

bool cnStruct::HasValue(const std::string &fieldName) const
{
    std::lock_guard<std::mutex> lck(mLock);
    auto it = mValues.find(fieldName);
    if (it == mValues.end()) {
        return false;
    }
    return true;
}

void cnStruct::DeleteValue(const std::string &fieldName)
{
    std::lock_guard<std::mutex> lck(mLock);
    auto it = mValues.find(fieldName);
    if (it == mValues.end()) {
        return;
    }
    mValues.erase(it);
}

std::vector<std::string> cnStruct::GetValueNames() const
{
    std::lock_guard<std::mutex> lck(mLock);
    std::vector<std::string> vec;
    for (auto it = mValues.begin(); it != mValues.end(); ++it) {
        vec.push_back(it->first);
    }
    return vec;
}

size_t cnStruct::GetValueCount() const
{
    std::lock_guard<std::mutex> lck(mLock);
    return mValues.size();
}

bool cnStruct::SetUserData(const std::string &fieldName, cnData* data)
{
    DeleteUserData(fieldName);
    
    std::lock_guard<std::mutex> lck(mLock);
    auto it = mUserDatas.find(fieldName);
    if (it != mUserDatas.end()) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        }
    }
    mUserDatas[fieldName] = data;
    return true;
}

cnData* cnStruct::GetUserData(const std::string &fieldName) const
{
    std::lock_guard<std::mutex> lck(mLock);
    auto it = mUserDatas.find(fieldName);
    if (it != mUserDatas.end()) {
        return it->second;
    }
    return nullptr;
}

cnData* cnStruct::StealUserData(const std::string &fieldName)
{
    std::lock_guard<std::mutex> lck(mLock);
    auto it = mUserDatas.find(fieldName);
    if (it == mUserDatas.end()) {
        return nullptr;
    }
    auto *frame = it->second;
    mUserDatas.erase(it);
    return frame;
}

bool cnStruct::HasUserData(const std::string &fieldName) const
{
    std::lock_guard<std::mutex> lck(mLock);
    auto it = mUserDatas.find(fieldName);
    return it != mUserDatas.end();
}

void cnStruct::DeleteUserData(const std::string &fieldName)
{
    auto *data = StealUserData(fieldName);
    if (data != nullptr) {
        delete data;
        data = nullptr;
    }
}

std::vector<std::string> cnStruct::GetUserDataNames() const
{
    std::lock_guard<std::mutex> lck(mLock);
    std::vector<std::string> vec;
    for (auto it = mUserDatas.begin(); it != mUserDatas.end(); ++it) {
        vec.push_back(it->first);
    }
    return vec;
}

size_t cnStruct::GetUserDataCount() const {
    std::lock_guard<std::mutex> lck(mLock);
    return mUserDatas.size();
}

void cnStruct::Release()
{
    std::lock_guard<std::mutex> lck(mLock);
    mValues.clear();
    for (auto& it : mUserDatas) {
        if (it.second != nullptr) {
            delete it.second;
            it.second = nullptr;
        }
    }
    mUserDatas.clear();
}

cnStruct* cnStruct::Clone() const
{
    cnStruct *structure = new cnStruct();
    structure->CopyFrom(*this);
    return structure;
}

void cnStruct::CopyFrom(const cnStruct &cncvStructure)
{
    std::lock_guard<std::mutex> rlck(cncvStructure.mLock);
    for (const auto &it : cncvStructure.mValues) {
        SetStringValue(it.first, it.second);
    }
    for (const auto &it : cncvStructure.mUserDatas) {
        SetUserData(it.first, it.second->Clone());
    }
}