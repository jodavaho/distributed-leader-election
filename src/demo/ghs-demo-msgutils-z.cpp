#include "ghs-demo-msgutils.hpp"
#include "miniz.h"
#include <assert.h>


Msg from_bytes(uint8_t *b, size_t c_sz)
{
  Msg m;
  auto bp = (unsigned char*) &b[0];
  auto mp = (unsigned char*) &m;
  mz_ulong mz_c_sz = c_sz;
  mz_ulong mz_m_sz = sizeof(m);
  int retval = uncompress(mp,&mz_m_sz,bp,mz_c_sz);
  if (retval!=Z_OK){
    assert(false && "miniz had an error!");
  }
  return m;
}

void to_bytes(const Msg&m, uint8_t* b, size_t &bsz){

  auto bp = (unsigned char*) &b[0];
  auto mp = (unsigned char*) &m;
  mz_ulong mz_b_sz = bsz;
  mz_ulong mz_m_sz = sizeof(m);

  int retval =compress(bp,&mz_b_sz,mp,mz_m_sz);
  if (retval!=Z_OK){
    assert(false && "miniz had an error!");
  }

  bsz = mz_b_sz;

}

