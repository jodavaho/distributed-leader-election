#ifndef GHS_DEMO_MSGUTILS
#define GHS_DEMO_MSGUTILS

#include "ghs/msg.hpp"

//quick and dirty static-sized message move
template <std::size_t NUM>
Msg from_bytes(uint8_t b[NUM]){
  static_assert(NUM>=GHS_MAX_MSG_SZ,"from_bytes only accepts statically-sized arrays >= sizeof(Msg)");
  Msg r;
  void* rp = (void*)&r;
  memmove(rp,(void*)b,GHS_MAX_MSG_SZ);
  return r;
}

template <std::size_t NUM>
void to_bytes(const Msg&m, uint8_t b[NUM]){
#ifndef USE_COMPRESSION
  static_assert(NUM>=GHS_MAX_MSG_SZ,"from_bytes only accepts statically-sized arrays >= sizeof(Msg)");
  void* mp = (void*)&m;
  void* bp = (void*)&b[0];
  memmove(bp,mp,GHS_MAX_MSG_SZ);
  return;
#else
#endif

}

//non-static versions (w/ or w/o compression)
Msg from_bytes(uint8_t *b, size_t b_sz);
void to_bytes(const Msg&m, uint8_t* b, size_t &b_sz);

#endif 
