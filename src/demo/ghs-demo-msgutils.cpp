#include "ghs-demo-msgutils.h"

Msg from_bytes(unsigned char *b, size_t c_sz)
{
  Msg r;
  void* rp = (void*)&r;
  memmove(rp,(void*)b,MAX_MSG_SZ);
  return r;
}

void to_bytes(const Msg&m, unsigned char* b, size_t &bsz){

  void* mp = (void*)&m;
  void* bp = (void*)&b[0];
  memmove(bp,mp,MAX_MSG_SZ);
}

