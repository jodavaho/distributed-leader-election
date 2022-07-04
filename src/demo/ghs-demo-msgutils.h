/**
 *   Copyright (c) 2022 California Institute of Technology (“Caltech”). 
 *   U.S.  Government sponsorship acknowledged.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *    * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    *  Neither the name of Caltech nor its operating division, the Jet
 *    Propulsion Laboratory, nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * @file ghs-demo-msgutils.h
 * @brief Provides some convenience functions for dealing with demo::WireMessage buffers
 */
#ifndef GHS_DEMO_MSGUTILS
#define GHS_DEMO_MSGUTILS

#include "ghs/msg.h"
#include <cstring>

using dle::GhsMsg;
using dle::MAX_MSG_SZ;

/**
 * @brief quick and dirty static-sized message move
 *
 * @param NUM a std::size_t that determines the size of the buffer to construct a dle::GhsMsg from
 * @param b an unsigned char array of size NUM (or larger)
 */
template <std::size_t NUM>
GhsMsg from_bytes(unsigned char b[NUM]){
  static_assert(NUM>=MAX_MSG_SZ,"from_bytes only accepts statically-sized arrays >= sizeof(GhsMsg)");
  GhsMsg r;
  void* rp = (void*)&r;
  memmove(rp,(void*)b,MAX_MSG_SZ);
  return r;
}

/**
 * @brief quick and dirty static-sized message move
 *
 * @param NUM a std::size_t that determines the size of the buffer to construct a dle::GhsMsg from
 * @param m a dle::GhsMsg to convert to bytes and copy to the buffer `b`
 * @param b an unsigned char array of size NUM (or larger)
 */
template <std::size_t NUM>
void to_bytes(const GhsMsg&m, unsigned char b[NUM]){
  static_assert(NUM>=MAX_MSG_SZ,"from_bytes only accepts statically-sized arrays >= sizeof(GhsMsg)");
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
 * @param b an unsigned char pointer to construct a dle::GhsMsg from
 * @param sz a size_t that provides the size of the buffer
 * @return dle::GhsMsg constructed from the buffer bytes
 */
GhsMsg from_bytes(unsigned char *b, size_t b_sz);

/**
 *
 * @brief non-static versions (w/ or w/o compression, depending on compile flags)
 *
 * Note that it will verify that the passed-in size is large enough to use.
 *
 * @param m an dle::GhsMsg to convert to types
 * @param b an unsigned char pointer to copy the msg bytes into
 * @param sz a size_t that provides the size of the destination buffer
 */
void to_bytes(const GhsMsg&m, unsigned char* b, size_t &b_sz);

#endif 
