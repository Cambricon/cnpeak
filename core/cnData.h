#ifndef CNPIPE_CNDATA_H
#define CNPIPE_CNDATA_H

#include <string>

class cnData
{
public:
  cnData();
  virtual ~cnData();

  bool Malloc(size_t size);
  void *GetCPUPtr() const;
  void *GetMLUPtr() const;
  void *StealMLUPtr(size_t &len);
  void *StealCPUPtr(size_t &len);
  bool SetToCPU(void *ptr, int len);
  bool SetToMLU(void *ptr, int len);
  bool Release();

  size_t GetLength() const;
  virtual cnData* Clone() const;

private:
  bool MLUToCPU(const void *src, void *&dst, size_t len) const;
  bool MLUToMLU(const void *src, void *&dst, size_t len) const;
  bool CPUToMLU(const void *src, void *&dst, size_t len) const;

private:
  class _Data;
  _Data *mData = nullptr;
};
#endif //CNPIPE_CNDATA_H
