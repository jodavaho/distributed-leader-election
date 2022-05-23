#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest/doctest.h"
#include "ghs/msg.hpp"
#include "ghs-demo-msgutils.hpp"

TEST_CASE("to_bytes"){
  InPartPayload pld{2,3};
  Msg ghs_msg = pld.to_msg(0,1);
  CHECK_EQ(ghs_msg.to, 0);
  CHECK_EQ(ghs_msg.from, 1);
  CHECK_EQ(ghs_msg.data.in_part.leader,2);
  CHECK_EQ(ghs_msg.data.in_part.level,3);
  unsigned char buf[GHS_MAX_MSG_SZ];
  to_bytes<GHS_MAX_MSG_SZ>(ghs_msg, buf);
  auto ghs_again = from_bytes<GHS_MAX_MSG_SZ>(buf);
  CHECK_EQ(ghs_again.type, IN_PART);
  CHECK_EQ(ghs_again.to, 0);
  CHECK_EQ(ghs_again.from, 1);
  CHECK_EQ(ghs_again.data.in_part.leader,2);
  CHECK_EQ(ghs_again.data.in_part.level,3);
}

