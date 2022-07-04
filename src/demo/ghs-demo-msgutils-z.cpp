/**
 *   @copyright 
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
 *
 * @file ghs-demo-msgutils-z.cpp
 *
 */
#include "ghs-demo-msgutils.h"
#include "miniz.h"
#include <assert.h>


GhsMsg from_bytes(uint8_t *b, size_t c_sz)
{
  GhsMsg m;
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

void to_bytes(const GhsMsg&m, uint8_t* b, size_t &bsz){

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

