/**
 * @file ghs-demo-msgutils.h
 * @brief Provides some convenience functions for dealing with demo::WireMessage buffers
 */
#ifndef GHS_DEMO_MSGUTILS
#define GHS_DEMO_MSGUTILS

#include "ghs/msg.h"
#include <cstring>

using le::ghs::Msg;
using le::ghs::MAX_MSG_SZ;

/**
 * @brief quick and dirty static-sized message move
 *
 * @param NUM a std::size_t that determines the size of the buffer to construct a le::ghs::Msg from
 * @param b an unsigned char array of size NUM (or larger)
 */
template <std::size_t NUM>
Msg from_bytes(unsigned char b[NUM]){
  static_assert(NUM>=MAX_MSG_SZ,"from_bytes only accepts statically-sized arrays >= sizeof(Msg)");
  Msg r;
  void* rp = (void*)&r;
  memmove(rp,(void*)b,MAX_MSG_SZ);
  return r;
}

/**
 * @brief quick and dirty static-sized message move
 *
 * @param NUM a std::size_t that determines the size of the buffer to construct a le::ghs::Msg from
 * @param m a le::ghs::Msg to convert to bytes and copy to the buffer `b`
 * @param b an unsigned char array of size NUM (or larger)
 */
template <std::size_t NUM>
void to_bytes(const Msg&m, unsigned char b[NUM]){
  static_assert(NUM>=MAX_MSG_SZ,"from_bytes only accepts statically-sized arrays >= sizeof(Msg)");
  void* mp = (void*)&m;
  void* bp = (void*)&b[0];
  memmove(bp,mp,MAX_MSG_SZ);
  return;
}

/**
 *
 * @brief non-static versions (w/ or w/o compression, depending on compile flags)
 *
 * Note that it will verify that the passed-in size is large enough to use.
 *
 * @param b an unsigned char pointer to construct a le::ghs::Msg from
 * @param sz a size_t that provides the size of the buffer
 * @return le::ghs::Msg constructed from the buffer bytes
 */
Msg from_bytes(unsigned char *b, size_t b_sz);

/**
 *
 * @brief non-static versions (w/ or w/o compression, depending on compile flags)
 *
 * Note that it will verify that the passed-in size is large enough to use.
 *
 * @param m an le::ghs::Msg to convert to types
 * @param b an unsigned char pointer to copy the msg bytes into
 * @param sz a size_t that provides the size of the destination buffer
 */
void to_bytes(const Msg&m, unsigned char* b, size_t &b_sz);

#endif 
