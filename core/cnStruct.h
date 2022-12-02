#ifndef CNPIPE_CNSTRUCT_H
#define CNPIPE_CNSTRUCT_H

#include <map>
#include <string>
#include <vector>
#include <mutex>
#include "cnData.h"

template <class Type> 
Type stringToNum(const std::string& str){ 
    std::istringstream iss(str); 
    Type num; 
    iss >> num; 
    return num; 
}

class cnStruct
{
private:
    mutable std::mutex mLock;
    std::map<std::string, std::string> mValues;
    std::map<std::string, cnData*> mUserDatas;
public:
    cnStruct(/* args */);
    ~cnStruct();

    bool GetStringValue(const std::string &fieldName, std::string& value) const;
    bool SetStringValue(const std::string &fieldName, const std::string &value);

    template<typename T>
    bool GetValue(const char* fieldName, T& value) {
        return GetStringValue(fieldName, stringToNum<T>(value));
    }

    template<typename T>
    bool SetValue(const char* fieldName, const T value) {
        return SetStringValue(fieldName, std::to_string(value));
    }

    bool HasValue(const std::string &fieldName) const;
    void DeleteValue(const std::string &fieldName);
    
    std::vector<std::string> GetValueNames() const;
    size_t GetValueCount() const;

    bool SetUserData(const std::string &fieldName, cnData* data);
    cnData* GetUserData(const std::string &fieldName) const;

    bool HasUserData(const std::string &fieldName) const;
    void DeleteUserData(const std::string &fieldName);

    std::vector<std::string> GetUserDataNames() const;
    size_t GetUserDataCount() const;
    cnData* StealUserData(const std::string &fieldName);

    void Release();

    cnStruct* Clone() const;

    void CopyFrom(const cnStruct &cncvStructure);
    
    
};
#endif //CNPIPE_CNSTRUCT_H
