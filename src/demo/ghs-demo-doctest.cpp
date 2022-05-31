#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest/doctest.h"
#include "ghs/msg.h"
#include "ghs-demo-msgutils.h"
#include "ghs-demo-config.h"
#include "ghs-demo-comms.h"

demo::Config get_cfg(int a=4){
  demo::Config ret;
  ret.my_id=0;
  ret.n_agents=a;
  return ret;
}

using le::ghs::Msg;
using le::ghs::InPartPayload;

TEST_CASE("to_bytes"){
  InPartPayload pld{2,3};
  Msg ghs_msg = pld.to_msg(0,1);
  CHECK_EQ(ghs_msg.to, 0);
  CHECK_EQ(ghs_msg.from, 1);
  CHECK_EQ(ghs_msg.data.in_part.leader,2);
  CHECK_EQ(ghs_msg.data.in_part.level,3);

  using le::ghs::MAX_MSG_SZ;
  unsigned char buf[MAX_MSG_SZ];
  to_bytes<MAX_MSG_SZ>(ghs_msg, buf);
  auto ghs_again = from_bytes<MAX_MSG_SZ>(buf);
  CHECK_EQ(ghs_again.type, le::ghs::Msg::IN_PART);
  CHECK_EQ(ghs_again.to, 0);
  CHECK_EQ(ghs_again.from, 1);
  CHECK_EQ(ghs_again.data.in_part.leader,2);
  CHECK_EQ(ghs_again.data.in_part.level,3);
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
