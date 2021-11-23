#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ghs.hpp"
#include "msg.hpp"
#include <deque>

TEST_CASE("test typecast")
{

  std::deque<Msg> buf;
  GHS_State s(0,4);
  size_t sent = s.typecast(EdgeStatus::UNKNOWN, Msg::Type::SRCH, {}, &buf);
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({0,1,EdgeStatus::UNKNOWN,1});
  sent = s.typecast(EdgeStatus::UNKNOWN, Msg::Type::SRCH, {}, &buf);
  //got one
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 1);
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({0,2,EdgeStatus::MST,1});
  //add edge to node 2 of wrong type
  s.set_parent_edge({0,2,EdgeStatus::MST,1});
  //look for unknown
  sent = s.typecast(EdgeStatus::UNKNOWN, Msg::Type::SRCH, {}, &buf);
  //only one still, right?
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 1); //<-- not 2!
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({0,3,EdgeStatus::MST,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {}, &buf);
  //Now got one here too
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 3);
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

}

TEST_CASE("test mst_broadcast")
{

  std::deque<Msg> buf;
  GHS_State s(0,4);
  size_t sent = s.mst_broadcast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({0,1,EdgeStatus::UNKNOWN,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to, still
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  s.set_edge({0,2,EdgeStatus::MST,1});
  s.set_parent_edge({0,2,EdgeStatus::MST,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to, still, right!?
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  s.set_edge({0,3,EdgeStatus::MST,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {}, &buf);
  //Now got one
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 3);
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

}

TEST_CASE("test mst_convergecast")
{

  std::deque<Msg> buf;
  GHS_State s(0,4);
  size_t sent;
  sent =   s.mst_convergecast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({0,1,EdgeStatus::UNKNOWN,1});
  sent = s.mst_convergecast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to, still
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({0,2,EdgeStatus::MST,1});
  sent = s.mst_convergecast(Msg::Type::SRCH, {}, &buf);
  //Still can't send to children MST
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({0,3,EdgeStatus::MST,1});
  s.set_parent_edge({0,3,EdgeStatus::MST,1});
  sent = s.mst_convergecast(Msg::Type::SRCH, {}, &buf);
  //Only to parents!
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 3);
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

}

TEST_CASE("test start_round() on leader, unknown peers")
{
  std::deque<Msg> buf;
  GHS_State s(0,3);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,UNKNOWN,1});
  s.set_edge({0, 2,UNKNOWN,1});
  s.start_round(&buf);
  CHECK_EQ(buf.size(),2);
  while(buf.size()>0){ 
    auto m = buf.front();
    CHECK_EQ(m.type, Msg::Type::IN_PART);
    buf.pop_front();
  }
}

TEST_CASE("test start_round() on leader, mst peers")
{
  std::deque<Msg> buf;
  GHS_State s(0,3);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,MST,1});
  s.set_edge({0, 2,MST,1});
  s.start_round(&buf);

  CHECK_EQ(buf.size(),2);
  auto m = buf.front();
  CHECK_EQ(m.type, Msg::Type::SRCH);
  buf.pop_front();
  m = buf.front();
  CHECK_EQ(m.type, Msg::Type::SRCH);
  buf.pop_front();
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("test start_round() on leader, discarded peers")
{
  std::deque<Msg> buf;
  GHS_State s(0,3);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,DELETED,1});
  s.set_edge({0, 2,DELETED,1});
  s.start_round(&buf);
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("test start_round() on leader, mixed peers")
{
  std::deque<Msg> buf;
  GHS_State s(0,4);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,DELETED,1});
  s.set_edge({0, 2,MST,1});
  s.set_edge({0, 3,UNKNOWN,1});
  s.start_round(&buf);

  CHECK_EQ(buf.size(),2);

  auto m = buf.front();
  buf.pop_front();
  CHECK_EQ(2, m.to);
  CHECK_EQ(0,m.from);
  CHECK_EQ(m.type,Msg::Type::SRCH);

  m = buf.front();
  buf.pop_front();
  CHECK_EQ(3, m.to);
  CHECK_EQ(0, m.from);
  CHECK_EQ(m.type,Msg::Type::IN_PART);

  CHECK_EQ(buf.size(),0);
}

TEST_CASE("test start_round() on non-leader")
{
  std::deque<Msg> buf;
  //set id to 0, and 4 total nodes
  GHS_State s(0,4);
  //set leader to 1, level to 0 (ignored)
  s.set_partition({1,0});

  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,DELETED,1});
  s.set_edge({0, 2,MST,1});
  s.set_edge({0, 3,UNKNOWN,1});
  s.start_round(&buf);

  //better have done nothing!
  CHECK_EQ(buf.size(),0);

}

TEST_CASE("test process_srch() checks recipient"){

  std::deque<Msg> buf;
  //set id to 0, and 4 total nodes
  GHS_State s(0,4);
  //set leader to 1, level to 0 (ignored)
  s.set_partition({1,0});
  //from rando
  CHECK_THROWS_AS(s.process( Msg{Msg::Type::SRCH,0,2,{}}, &buf), const std::invalid_argument&);
  //from leader is ok
  CHECK_NOTHROW(s.process( Msg{Msg::Type::SRCH,0,1,{}}, &buf));
  //from self not allowed
  CHECK_THROWS_AS(s.process( Msg{Msg::Type::SRCH,0,0,{}}, &buf), const std::invalid_argument&);
    
}

TEST_CASE("test process({Msg::Type::SRCH,0,1,{}},), unknown peers")
{
  std::deque<Msg> buf;
  GHS_State s(0,3);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,UNKNOWN,1});
  s.set_edge({0, 2,UNKNOWN,1});
  s.start_round(&buf);
  CHECK_EQ(buf.size(),2);
  while(buf.size()>0){ 
    auto m = buf.front();
    CHECK_EQ(m.type, Msg::Type::IN_PART);
    buf.pop_front();
  }
}

TEST_CASE("test process_srch,  mst peers")
{
  std::deque<Msg> buf;
  GHS_State s(0,3);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,MST,1});
  s.set_edge({0, 2,MST,1});
  s.process({Msg::Type::SRCH,0,0,{}},&buf);

  CHECK_EQ(buf.size(),2);
  auto m = buf.front();
  CHECK_EQ(m.type, Msg::Type::SRCH);
  buf.pop_front();
  m = buf.front();
  CHECK_EQ(m.type, Msg::Type::SRCH);
  buf.pop_front();
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("test process_srch, discarded peers")
{
  std::deque<Msg> buf;
  GHS_State s(0,3);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,DELETED,1});
  s.set_edge({0, 2,DELETED,1});
  s.process({Msg::Type::SRCH,0,0,{}},&buf);
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("test process_srch, mixed peers")
{
  std::deque<Msg> buf;
  GHS_State s(0,4);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,DELETED,1});
  s.set_edge({0, 2,MST,1});
  s.set_edge({0, 3,UNKNOWN,1});
  s.process({Msg::Type::SRCH,0,0,{}},&buf);

  CHECK_EQ(buf.size(),2);

  auto m = buf.front();
  buf.pop_front();
  CHECK_EQ(2, m.to);
  CHECK_EQ(0,m.from);
  CHECK_EQ(m.type,Msg::Type::SRCH);

  m = buf.front();
  buf.pop_front();
  CHECK_EQ(3, m.to);
  CHECK_EQ(0, m.from);
  CHECK_EQ(m.type,Msg::Type::IN_PART);

  CHECK_EQ(buf.size(),0);
}

TEST_CASE("test process_srch, mixed peers, with parent link")
{
  std::deque<Msg> buf;
  GHS_State s(0,4);
  s.set_partition({3,0});
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({0, 1,DELETED,1});
  s.set_edge({0, 2,MST,1});
  s.set_edge({0, 3,MST,1});
  s.set_parent_edge( {0,3,MST,1} );
  CHECK_NOTHROW(s.process({Msg::Type::SRCH,0,3,{}},&buf));

  CHECK_EQ(buf.size(),1);

  auto m = buf.front();
  buf.pop_front();
  CHECK_EQ(2, m.to);
  CHECK_EQ(0,m.from);
  CHECK_EQ(m.type,Msg::Type::SRCH);

  CHECK_EQ(buf.size(),0);
  //now from non-parent
  CHECK_THROWS_AS(s.process({Msg::Type::SRCH,0,2,{}},&buf), const std::invalid_argument&);
}

TEST_CASE("test ghs_worst_possible_edge()")
{
  Edge edge = ghs_worst_possible_edge();
  CHECK(edge.peer == -1);
  CHECK(edge.root== -1);
  CHECK(edge.status== UNKNOWN);
  Edge y = Edge{1,4,UNKNOWN, 1000};
  CHECK(y.peer == 4);
  CHECK(y.root== 1);
  CHECK(y.metric_val== 1000);
  y = edge;
  CHECK_EQ(y.metric_val, std::numeric_limits<int>::max());
}

TEST_CASE("test process_srch_ret throws when not waiting")
{

  std::deque<Msg> buf;
  GHS_State s(0,4);
  CHECK_THROWS_AS(s.process( Msg{Msg::Type::SRCH_RET,0,1,{}}, &buf), const std::invalid_argument&);
}

TEST_CASE("test process_srch_ret, one peer, no edge found ")
{

  std::deque<Msg> buf;
  GHS_State s(0,4);
  //set up one peer, node 1
  s.set_edge({0, 1,MST,1});

  //start it
  s.start_round(&buf);

  CHECK_EQ(buf.size(),1); // SRCH
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);
  buf.pop_front();
  CHECK_EQ(s.waiting_count(),1);//the MST and UNKNOWN edges
  //pretend node 1 returned a bad edge
  auto bad_edge = ghs_worst_possible_edge();

  //send a return message 
  auto srch_ret_msg = Msg{Msg::Type::SRCH_RET,0,1, {1,2,bad_edge.metric_val} };

  CHECK_NOTHROW(s.process( srch_ret_msg, &buf));
  CHECK_EQ(s.waiting_count(),0);
  //did not accept their edge
  CHECK_EQ(s.mwoe().root, 0);
  CHECK_EQ(s.mwoe().peer, -1);
  CHECK_EQ(s.mwoe().metric_val, std::numeric_limits<int>::max());

  //check the response (leaders either add edges or call for elections)
  CHECK_EQ(buf.size(),1);
  //in this case, we didn't create a good edge, so they assume there's none avail
  CHECK_EQ(buf.front().type, Msg::Type::ELECTION);

}

TEST_CASE("test process_srch_ret, one peer, edge found ")
{

  std::deque<Msg> buf;
  GHS_State s(0,4);
  //set up one peer, node 1
  s.set_edge({0, 1,MST,1});
  s.set_partition({0,0});

  //start it
  s.start_round(&buf);

  CHECK_EQ(buf.size(),1); // SRCH
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);
  buf.pop_front();
  CHECK_EQ(s.waiting_count(),1);
  CHECK_EQ(s.mwoe().root, 0);
  CHECK_EQ(s.mwoe().peer, -1);
  CHECK_EQ(s.mwoe().metric_val, std::numeric_limits<int>::max());
  
  //pretend node 1 returned a good edge (1-->2, wt=0)
  //send a return message 
  auto srch_ret_msg = Msg{Msg::Type::SRCH_RET,0,1, {1,2,1} };

  CHECK_NOTHROW(s.process( srch_ret_msg, &buf));

  //that should be the repsponse it was looking for
  CHECK_EQ(s.waiting_count(),0);
  //accepted their edge
  CHECK_EQ(s.mwoe().root, 1);
  CHECK_EQ(s.mwoe().peer, 2);
  CHECK_EQ(s.mwoe().metric_val, 1);

  //check the response (leaders either add edges or call for elections)
  CHECK_EQ(buf.size(),1);
  //in this case, we have a candidate, that should join_us
  CHECK_EQ(buf.front().type, Msg::Type::JOIN_US);

}

TEST_CASE("test process_srch_ret, one peer, not leader")
{

  std::deque<Msg> buf;
  GHS_State s(0,4);
  //set up one peer, node 1
  s.set_edge({0, 1,MST,1});
  s.set_edge({0, 2,MST,1});
  s.set_parent_edge( {0,1,MST,1} );
  s.set_partition( {1,0} );

  //start it by getting a search from leader to node 0
  s.process({ Msg::Type::SRCH,0,1,{}  }, &buf);
  CHECK_EQ(s.mwoe().root, 0);
  CHECK_EQ(s.mwoe().peer, -1);
  CHECK_EQ(s.mwoe().metric_val, std::numeric_limits<int>::max());
  CHECK_EQ(buf.size(),1); // SRCH to 2
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);
  CHECK_EQ(buf.front().to, 2);
  buf.pop_front();
  CHECK_EQ(s.waiting_count(),1);//waiting on node 2
  //pretend node 2 returned a good edge (from 2 to 3 with wt 0) back to node 0
  auto srch_ret_msg = Msg{Msg::Type::SRCH_RET,0,2, {2,3,1} };

  CHECK_NOTHROW(s.process( srch_ret_msg, &buf));
  CHECK_EQ(s.waiting_count(),0);
  CHECK_EQ(s.mwoe().root, 2);
  CHECK_EQ(s.mwoe().peer, 3);
  CHECK_EQ(s.mwoe().metric_val, 1);

  //check the response (leaders either add edges or call for elections)
  CHECK_EQ(buf.size(),1);
  //in this case, we have to tell parent
  CHECK_EQ(buf.front().type, Msg::Type::SRCH_RET);
  CHECK_EQ(buf.front().to, 1);

}
