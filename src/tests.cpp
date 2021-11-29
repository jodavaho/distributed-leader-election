#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ghs.hpp"
#include "msg.hpp"
#include <deque>
#include <optional>

TEST_CASE("test set_edge_status"){
  GhsState s(0);
  CHECK_EQ(1, s.set_edge( {1,0,DELETED, 1}));
  CHECK_EQ(0, s.set_edge( {1,0,UNKNOWN, 1}));

  std::optional<Edge> e;
  CHECK_NOTHROW(s.get_edge(2));
  CHECK( !e );
  CHECK_NOTHROW(e = s.get_edge(1));
  CHECK( e ); 
  CHECK_EQ(e->peer, 1);
  CHECK_EQ(e->root,0);
  CHECK_EQ(e->status, UNKNOWN);
  CHECK_EQ(e->metric_val, 1);

  CHECK_NOTHROW(s.set_edge_status(1,MST));
  CHECK_NOTHROW(e = s.get_edge(1));
  CHECK( e ); 
  CHECK_EQ(e->peer, 1);
  CHECK_EQ(e->root,0);
  CHECK_EQ(e->status, MST);
  CHECK_EQ(e->metric_val, 1);

  CHECK_THROWS_AS( s.set_edge_status(2,MST), std::invalid_argument & );
}

TEST_CASE("test typecast")
{

  std::deque<Msg> buf;
  GhsState s(0);
  size_t sent = s.typecast(EdgeStatus::UNKNOWN, Msg::Type::SRCH, {}, &buf);
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({1,0,EdgeStatus::UNKNOWN,1});
  sent = s.typecast(EdgeStatus::UNKNOWN, Msg::Type::SRCH, {}, &buf);
  //got one
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 1);
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({2,0,EdgeStatus::MST,1});
  //add edge to node 2 of wrong type
  s.set_parent_edge({2,0,EdgeStatus::MST,1});
  //look for unknown
  sent = s.typecast(EdgeStatus::UNKNOWN, Msg::Type::SRCH, {}, &buf);
  //only one still, right?
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 1); //<-- not 2!
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({3,0,EdgeStatus::MST,1});
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
  GhsState s(0);
  size_t sent = s.mst_broadcast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({1,0,EdgeStatus::UNKNOWN,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to, still
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  s.set_edge({2,0,EdgeStatus::MST,1});
  s.set_parent_edge({2,0,EdgeStatus::MST,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to, still, right!?
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  s.set_edge({3,0,EdgeStatus::MST,1});
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
  GhsState s(0);
  size_t sent;
  sent =   s.mst_convergecast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({1,0,EdgeStatus::UNKNOWN,1});
  sent = s.mst_convergecast(Msg::Type::SRCH, {}, &buf);
  //nobody to send to, still
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({2,0,EdgeStatus::MST,1});
  sent = s.mst_convergecast(Msg::Type::SRCH, {}, &buf);
  //Still can't send to children MST
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({3,0,EdgeStatus::MST,1});
  s.set_parent_edge({3,0,EdgeStatus::MST,1});
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
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,UNKNOWN,1});
  s.set_edge({2, 0,UNKNOWN,1});
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
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,MST,1});
  s.set_edge({2, 0,MST,1});
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
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,DELETED,1});
  s.start_round(&buf);
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("test start_round() on leader, mixed peers")
{
  std::deque<Msg> buf;
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,MST,1});
  s.set_edge({3, 0,UNKNOWN,1});
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
  GhsState s(0);
  //set leader to 1, level to 0 (ignored)
  s.set_partition({1,0});

  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,MST,1});
  s.set_edge({3, 0,UNKNOWN,1});
  s.start_round(&buf);

  //better have done nothing!
  CHECK_EQ(buf.size(),0);

}

TEST_CASE("test process_srch() checks recipient"){

  std::deque<Msg> buf;
  //set id to 0, and 4 total nodes
  GhsState s(0);
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
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,UNKNOWN,1});
  s.set_edge({2, 0,UNKNOWN,1});
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
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,MST,1});
  s.set_edge({2, 0,MST,1});
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
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,DELETED,1});
  s.process({Msg::Type::SRCH,0,0,{}},&buf);
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("test process_srch, mixed peers")
{
  std::deque<Msg> buf;
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,MST,1});
  s.set_edge({3, 0,UNKNOWN,1});
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
  GhsState s(0);
  s.set_partition({3,0});
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,MST,1});
  s.set_edge({3, 0,MST,1});
  s.set_parent_edge( {3,0,MST,1} );
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

TEST_CASE("Guard against Edge refactoring"){
  Edge y = Edge{4,1,UNKNOWN, 1000};
  CHECK(y.peer == 4);
  CHECK(y.root== 1);
  CHECK(y.metric_val== 1000);
}

TEST_CASE("Guard against Msg refactoring"){
  Msg m = Msg{(Msg::Type)1, 4,1,{33, 17}};
  CHECK_EQ(m.type, Msg::Type::SRCH);
  CHECK_EQ(m.to, 4);
  CHECK_EQ(m.from, 1);
  CHECK_EQ(m.data.size(),2);
  CHECK_EQ(m.data[0], 33);
  CHECK_EQ(m.data[1], 17);

}

TEST_CASE("test ghs_worst_possible_edge()")
{
  Edge edge = ghs_worst_possible_edge();
  CHECK(edge.peer == -1);
  CHECK(edge.root== -1);
  CHECK(edge.status== UNKNOWN);
  Edge y = Edge{4,1,UNKNOWN, 1000};
  CHECK(y.peer == 4);
  CHECK(y.root== 1);
  CHECK(y.metric_val== 1000);
  y = edge;
  CHECK_EQ(y.metric_val, std::numeric_limits<int>::max());
}

TEST_CASE("test process_srch_ret throws when not waiting")
{

  std::deque<Msg> buf;
  GhsState s(0);
  CHECK_THROWS_AS(s.process( Msg{Msg::Type::SRCH_RET,0,1,{}}, &buf), const std::invalid_argument&);
}

TEST_CASE("test process_srch_ret, one peer, no edge found ")
{

  std::deque<Msg> buf;
  GhsState s(0);
  //set up one peer, node 1
  s.set_edge({1, 0,MST,1});

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
  GhsState s(0);
  //set up one peer, node 1
  s.set_edge({1, 0,MST,1});
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
  auto srch_ret_msg = Msg{Msg::Type::SRCH_RET,0,1, {2,1,1} };

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
  GhsState s(0);
  //set up one peer, node 1
  s.set_edge({1, 0,MST,1});
  s.set_edge({2, 0,MST,1});
  s.set_parent_edge( {1,0,MST,1} );
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
  auto srch_ret_msg = Msg{Msg::Type::SRCH_RET,0,2, {3,2,1} };

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

TEST_CASE("test process_ack_part, happy-path"){
  GhsState s(0);
  std::deque<Msg> buf;
  std::optional<Edge> e;

  //create edge to 1
  int r;
  Edge e1 = {1,0,UNKNOWN,10};
  CHECK_EQ(1, e1.peer);
  CHECK_EQ(0, e1.root);
  CHECK_NOTHROW(r = s.set_edge( e1 ));
  CHECK_EQ(1,r);//<-- was it added?
  CHECK_NOTHROW( e = s.get_edge(1) );
  CHECK(e);
  CHECK_EQ(e->status, UNKNOWN);
  s.start_round(&buf);
  CHECK_EQ(1,s.waiting_count());
  CHECK_EQ(buf.size(),1);
  buf.pop_front(); 

  Msg m{Msg::Type::ACK_PART,0,1,{}};
  CHECK_EQ(m.from,1);//<-- code should modify using "from" field
  CHECK_NOTHROW(s.process(m,&buf));
  CHECK_NOTHROW( e=s.get_edge(1));
  CHECK(e);
  CHECK_EQ(e->status, DELETED);
}

TEST_CASE("test process_ack_part, not waiting for anyone"){
  GhsState s(0);
  std::deque<Msg> buf;
  std::optional<Edge> e;

  //create edge to 1
  int r;
  Edge e1 = {1,0,UNKNOWN,10};
  CHECK_EQ(1, e1.peer);
  CHECK_EQ(0, e1.root);
  CHECK_NOTHROW(r = s.set_edge( e1 ));
  CHECK_EQ(1,r);//<-- was it added?
  CHECK_NOTHROW( e = s.get_edge(1) );
  CHECK(e);
  CHECK_EQ(e->status, UNKNOWN);

  CHECK_EQ(0,s.waiting_count());
  Msg m{Msg::Type::ACK_PART,0,1,{}};
  CHECK_EQ(m.from,1);//<-- code should modify using "from" field
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument&);
  CHECK_NOTHROW( e=s.get_edge(1));
  CHECK(e);
  CHECK_EQ(e->status, UNKNOWN); //<--unmodified!
}

TEST_CASE("test process_ack_part, no edge"){
  GhsState s(0);
  std::deque<Msg> buf;
  std::optional<Edge> e;

  //create edge to 1
  Edge e1 = {1,0,UNKNOWN,10};
  CHECK_EQ(1, e1.peer);
  CHECK_EQ(0, e1.root);
  CHECK_NOTHROW( e = s.get_edge(1) );
  CHECK(!e);
  CHECK_EQ(0,s.waiting_count());

  Msg m{Msg::Type::ACK_PART,0,1,{}};
  CHECK_EQ(m.from,1);//<-- code should modify using "from" field
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument&);
  CHECK_NOTHROW( e=s.get_edge(1));
  CHECK(!e);
}

TEST_CASE("test process_ack_part, waiting, but not for sender"){
  GhsState s(0);
  std::deque<Msg> buf;
  std::optional<Edge> e;

  //create edge to 1
  Edge e1 = {1,0,UNKNOWN,10};
  Edge e2 = {2,0,UNKNOWN,10};
  CHECK_NOTHROW(s.set_edge( e1 ));
  CHECK_NOTHROW(s.set_edge( e2 ));
  s.start_round(&buf);
  CHECK_EQ(2,s.waiting_count());
  CHECK_EQ(buf.size(),2);
  buf.pop_front(); 
  buf.pop_front(); 

  Msg m{Msg::Type::ACK_PART,0,2,{}};
  CHECK_NOTHROW( e=s.get_edge(2));
  CHECK(e); //<-- valid edge
  CHECK_EQ(m.from,2);   //<-- code should modify using "from" field
  CHECK_NOTHROW(s.process(m,&buf));
  CHECK_EQ(1,s.waiting_count()); //<-- got data we need from 2
  //send msg again:
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument&);
}

TEST_CASE("test in_part, happy-path"){
  GhsState s(0);
  std::deque<Msg> buf;
  std::optional<Edge> e;
  //are you, node 0, in partition led by agent 1 with level 2? 
  s.process( Msg{Msg::Type::IN_PART,0,1,{1,2}}, &buf);
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(s.waiting_count(),0);
  //no, I am not
  CHECK_EQ(buf.front().type, Msg::Type::NACK_PART);
  buf.pop_front();

  //are you, node 0,  in partition led by agent 0 with level 2? 
  s.process( Msg{Msg::Type::IN_PART,0,1,{0,2}}, &buf);
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(s.waiting_count(),0);
  //yes, I am
  CHECK_EQ(buf.front().type, Msg::Type::ACK_PART);
  buf.pop_front();
}

TEST_CASE("test process_nack_part, happy-path"){

  GhsState s(0);
  std::deque<Msg> buf;
  Msg m;

  CHECK_NOTHROW(s.set_edge({1,0,UNKNOWN,10}));
  CHECK_NOTHROW(s.set_edge({2,0,UNKNOWN,20}));

  s.start_round(&buf);
  CHECK_EQ(buf.size(),2);
  buf.pop_front();buf.pop_front();

  CHECK_EQ(s.waiting_count(),2);
  m ={Msg::Type::NACK_PART, 0, 2, {}};
  s.process(m,&buf);
  CHECK_EQ(buf.size(),           0);
  CHECK_EQ(s.waiting_count(),    1);
  CHECK_EQ(s.mwoe().metric_val, 20);
  CHECK_EQ(s.mwoe().root,        0);
  CHECK_EQ(s.mwoe().peer,        2);
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument&);
  CHECK_EQ(buf.size(),0);

  m ={Msg::Type::NACK_PART, 0, 1, {}};
  s.process(m,&buf);
  CHECK_EQ(buf.size(),           1);
  CHECK_EQ(s.waiting_count(),    0);
  CHECK_EQ(s.mwoe().metric_val,  10);
  CHECK_EQ(s.mwoe().root,        0);
  CHECK_EQ(s.mwoe().peer,        1);
  //response should be to tell the other guy we're joining up
  CHECK_EQ(buf.front().type,  Msg::Type::JOIN_US);
  CHECK_EQ(buf.front().to,    s.mwoe().peer);
  CHECK_EQ(buf.front().from,  s.mwoe().root);
  CHECK_EQ(buf.front().from,  0);
  CHECK_EQ(s.get_edge(s.mwoe().peer)->status, MST);
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument&);
  CHECK_EQ(buf.size(),1);
  
}

TEST_CASE("test process_nack_part, not-leader"){

  GhsState s(0);
  std::deque<Msg> buf;
  Msg m;

  //add two outgoing unkonwn edges
  CHECK_NOTHROW(s.set_edge({1,0,UNKNOWN,10}));
  CHECK_NOTHROW(s.set_edge({2,0,UNKNOWN,20}));
  //add leader
  CHECK_NOTHROW(s.set_edge({3,0,MST,20}));
  CHECK_NOTHROW(s.set_parent_edge({3,0,MST,20}));
  CHECK_NOTHROW(s.set_partition({3,0}));

  //send the start message
  CHECK_NOTHROW(s.process(Msg{Msg::Type::SRCH,0,3,{}}, &buf));

  CHECK_EQ(buf.size(),2);
  buf.pop_front();
  buf.pop_front();

  CHECK_EQ(s.waiting_count(),2);
  //send msg from 2 --> not in partition
  m ={Msg::Type::NACK_PART, 0, 2, {}};
  s.process(m,&buf);
  CHECK_EQ(buf.size(),           0);
  CHECK_EQ(s.waiting_count(),    1);
  CHECK_EQ(s.mwoe().metric_val, 20);
  CHECK_EQ(s.mwoe().root,        0);
  CHECK_EQ(s.mwoe().peer,        2);
  //check error condition
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument&);
  CHECK_EQ(buf.size(),0);

  //send message from 1 --> not in partition
  m ={Msg::Type::NACK_PART, 0, 1, {}};
  s.process(m,&buf);
  CHECK_EQ(buf.size(),           1);
  CHECK_EQ(s.waiting_count(),    0);
  CHECK_EQ(s.mwoe().metric_val,  10);
  CHECK_EQ(s.mwoe().root,        0);
  CHECK_EQ(s.mwoe().peer,        1);

  //since we got both respones, but have leader
  //response should be to tell the boss
  CHECK_EQ(buf.front().type,  Msg::Type::SRCH_RET);
  CHECK_EQ(buf.front().to,    s.get_partition().leader);
  CHECK_EQ(buf.front().from,  0);
  //should not add edge yet
  CHECK_EQ(s.get_edge(s.mwoe().peer)->status, UNKNOWN);
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument&);
  CHECK_EQ(buf.size(),1);

}
