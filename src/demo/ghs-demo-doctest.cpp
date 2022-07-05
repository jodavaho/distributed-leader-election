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
 * @file ghs-demo-doctest.cpp
 *
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <dle/ghs_msg.h>
#include "ghs-demo-msgutils.h"
#include "ghs-demo-config.h"
#include "ghs-demo-comms.h"

using namespace dle;
using namespace dle::ghs_msg;

demo::Config get_cfg(int a=4){
  demo::Config ret;
  ret.my_id=0;
  ret.n_agents=a;
  return ret;
}

TEST_CASE("to_bytes"){
  auto ghs_msg = GhsMsg(0,1,InPartPayload {2,3});
  CHECK_EQ(ghs_msg.to(), 0);
  CHECK_EQ(ghs_msg.from(), 1);
  CHECK_EQ(ghs_msg.data().in_part.leader,2);
  CHECK_EQ(ghs_msg.data().in_part.level,3);

  unsigned char buf[MAX_MSG_SZ];
  to_bytes<MAX_MSG_SZ>(ghs_msg, buf);
  auto ghs_again = from_bytes<MAX_MSG_SZ>(buf);
  CHECK_EQ(ghs_again.type(), Type::IN_PART);
  CHECK_EQ(ghs_again.to(), 0);
  CHECK_EQ(ghs_again.from(), 1);
  CHECK_EQ(ghs_again.data().in_part.leader,2);
  CHECK_EQ(ghs_again.data().in_part.level,3);
}

TEST_CASE("unique_metric")
{
  CHECK_EQ(
      sym_metric(1,0,100) ,
      sym_metric(0,1,100) 
      );

  CHECK_LT(
      sym_metric(1,0,101) ,
      sym_metric(0,1,100) 
      );

  CHECK_GT(
      sym_metric(1,0,100) ,
      sym_metric(0,1,101) 
      );

  CHECK_NE(
      sym_metric(1,0,101) ,
      sym_metric(1,0,100) 
      );

  CHECK_NE(
      sym_metric(0,1,101) ,
      sym_metric(0,1,100) 
      );
}
