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
 * @file ghs-demo-doctest-miniz.cpp
 *
 */

#include "doctest/doctest.h"
#include <dle/ghs_msg.h>
#include "ghs-demo-msgutils.h"
#include "miniz.h"
#include <string>
#include <sstream>

TEST_CASE("miniz-understanding")
{
  std::string msg = "Helloooooo Miniz!!!!!!!!!!!!!";
  MESSAGE(msg);
  size_t len = msg.size()+1;
  
  unsigned char cbuf[256];
  mz_ulong sz = 256;

  auto retcode = compress(cbuf,&sz,(unsigned char*)msg.c_str(),len);
  CHECK_EQ(retcode, Z_OK);
  CHECK_LT(sz,256);
  CHECK_LT(sz,msg.size());

  unsigned char ubuf[256];
  mz_ulong sz_out = 256;

  retcode = uncompress(ubuf,&sz_out,cbuf,sz);
  CHECK_EQ(retcode,Z_OK);

  CHECK_GT(sz_out,sz);
  CHECK_EQ(sz_out,len);

  std::stringstream ss;
  ss<<(char*)&ubuf[0];
  MESSAGE(ss.str());
  CHECK(ss.str() == msg);

}

TEST_CASE("miniz-aint-magic")
{
  std::string msg = "Helloooooo Miniz!";
  MESSAGE(msg);
  size_t len = msg.size()+1;
  
  unsigned char cbuf[256];
  mz_ulong sz = 256;

  auto retcode = compress(cbuf,&sz,(unsigned char*)msg.c_str(),len);
  CHECK_EQ(retcode, Z_OK);
  CHECK_LT(sz,256);

  //SUPRISE:
  CHECK_GT(sz,msg.size());

  unsigned char ubuf[256];
  mz_ulong sz_out = 256;

  retcode = uncompress(ubuf,&sz_out,cbuf,sz);
  CHECK_EQ(retcode,Z_OK);

  //SUPRISE TOO:
  CHECK_LT(sz_out,sz);

  CHECK_EQ(sz_out,len);

  std::stringstream ss;
  ss<<(char*)&ubuf[0];
  MESSAGE(ss.str());
  CHECK(ss.str() == msg);

}
