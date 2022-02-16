#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ghs.hpp"
#include "msg.hpp"
#include <deque>
#include <optional>
#include <fstream>


GhsState get_state(AgentID my_id=0, size_t n_unknown=1, size_t n_deleted=0, size_t n_MST=0, bool is_root = true, bool waiting=false)
{
  GhsState s(my_id);
  AgentID id=1;
  std::optional<Edge> e;
  for (size_t i=0;i<n_unknown;i++, id++){
    REQUIRE_EQ(1, s.set_edge( {id,0,UNKNOWN, id}));
    REQUIRE_NOTHROW(e = s.get_edge(1));
    REQUIRE( e ); 
    REQUIRE_EQ(e->peer,id);
    REQUIRE_EQ(e->root,0);
    REQUIRE_EQ(e->status, UNKNOWN);
    REQUIRE_EQ(e->metric_val, id);
  }
  for (size_t i=0;i<n_deleted;i++, id++){
    REQUIRE_EQ(1, s.set_edge( {id,0,DELETED, id}));
    REQUIRE_NOTHROW(e = s.get_edge(id));
    REQUIRE( e ); 
    REQUIRE_EQ(e->peer,id);
    REQUIRE_EQ(e->root,0);
    REQUIRE_EQ(e->status, DELETED);
    REQUIRE_EQ(e->metric_val, id);
  }
  for (size_t i=0;i<n_MST;i++, id++){
    REQUIRE_EQ(1, s.set_edge( {id,0,MST, id}));
    REQUIRE_NOTHROW(e = s.get_edge(id));
    REQUIRE( e ); 
    REQUIRE_EQ(e->peer,id);
    REQUIRE_EQ(e->root,0);
    REQUIRE_EQ(e->status, MST);
    REQUIRE_EQ(e->metric_val, id);
  }
  if (!is_root){
    //just set the last MST link as the one to the root
    REQUIRE_NOTHROW(s.set_parent_edge( {id-1,0,MST,id-1} ));
    REQUIRE_NOTHROW(s.set_partition( {id-1, 0}));
  }
  return s;
}

TEST_CASE("unit-test get_state"){
  auto s = get_state(0,1,1,1,false,false);
}

TEST_CASE("unit-test set_edge_status"){
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

TEST_CASE("unit-test typecast")
{

  std::deque<Msg> buf;
  GhsState s(0);
  size_t sent = s.typecast(EdgeStatus::UNKNOWN, Msg::Type::SRCH, {0,0}, &buf);
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({1,0,EdgeStatus::UNKNOWN,1});
  sent = s.typecast(EdgeStatus::UNKNOWN, Msg::Type::SRCH, {0,0}, &buf);
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
  sent = s.typecast(EdgeStatus::UNKNOWN, Msg::Type::SRCH, {0,0}, &buf);
  //only one still, right?
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 1); //<-- not 2!
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({3,0,EdgeStatus::MST,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {0,0}, &buf);
  //Now got one here too
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 3);
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

}

TEST_CASE("unit-test mst_broadcast")
{

  std::deque<Msg> buf;
  GhsState s(0);
  size_t sent = s.mst_broadcast(Msg::Type::SRCH, {0,0}, &buf);
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({1,0,EdgeStatus::UNKNOWN,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {0,0}, &buf);
  //nobody to send to, still
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  s.set_edge({2,0,EdgeStatus::MST,1});
  s.set_parent_edge({2,0,EdgeStatus::MST,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {0,0}, &buf);
  //nobody to send to, still, right!?
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  s.set_edge({3,0,EdgeStatus::MST,1});
  sent = s.mst_broadcast(Msg::Type::SRCH, {0,0}, &buf);
  //Now got one
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 3);
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

}

TEST_CASE("unit-test mst_convergecast")
{

  std::deque<Msg> buf;
  GhsState s(0);
  size_t sent;
  sent =   s.mst_convergecast(Msg::Type::SRCH, {0,0}, &buf);
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({1,0,EdgeStatus::UNKNOWN,1});
  sent = s.mst_convergecast(Msg::Type::SRCH, {0,0}, &buf);
  //nobody to send to, still
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({2,0,EdgeStatus::MST,1});
  sent = s.mst_convergecast(Msg::Type::SRCH, {0,0}, &buf);
  //Still can't send to children MST
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop_front(); }

  s.set_edge({3,0,EdgeStatus::MST,1});
  s.set_parent_edge({3,0,EdgeStatus::MST,1});
  sent = s.mst_convergecast(Msg::Type::SRCH, {0,0}, &buf);
  //Only to parents!
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().to, 3);
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);

}

TEST_CASE("unit-test start_round() on leader, unknown peers")
{
  std::deque<Msg> buf;
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,UNKNOWN,1});
  s.set_edge({2, 0,UNKNOWN,1});
  CHECK_NOTHROW(s.start_round(&buf));
  CHECK_EQ(buf.size(),2);
  while(buf.size()>0){ 
    auto m = buf.front();
    CHECK_EQ(m.type, Msg::Type::IN_PART);
    buf.pop_front();
  }
}

TEST_CASE("unit-test start_round() on leader, mst peers")
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

TEST_CASE("unit-test start_round() on leader, discarded peers")
{
  std::deque<Msg> buf;
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,DELETED,1});
  s.start_round(&buf);
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("unit-test start_round() on leader, mixed peers")
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

TEST_CASE("unit-test start_round() on non-leader")
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

TEST_CASE("unit-test process_srch() checks recipient"){

  std::deque<Msg> buf;
  //set id to 0, and 4 total nodes
  GhsState s(0);
  //set leader to 1, level to 0 
  s.set_partition({1,0});
  s.set_edge({1,0,MST,1});
  //from rando
  CHECK_THROWS_AS(s.process( Msg{Msg::Type::SRCH,0,2,{1,0}}, &buf), const std::invalid_argument&);
  //from leader is ok
  CHECK_NOTHROW(s.process( Msg{Msg::Type::SRCH,0,1,{1,0}}, &buf));
  //from self not allowed
  CHECK_THROWS_AS(s.process( Msg{Msg::Type::SRCH,0,0,{1,0}}, &buf), const std::invalid_argument&);
    
}

TEST_CASE("unit-test process_srch, unknown peers")
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

TEST_CASE("unit-test process_srch,  mst peers")
{
  std::deque<Msg> buf;
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,MST,1});
  s.set_edge({2, 0,MST,1});
  s.set_edge({3, 0,MST,1});
  s.set_parent_edge({3,0,MST,1});
  s.process({Msg::Type::SRCH,0,3,{3,0}},&buf);

  CHECK_EQ(buf.size(),2);
  auto m = buf.front();
  CHECK_EQ(m.type, Msg::Type::SRCH);
  buf.pop_front();
  m = buf.front();
  CHECK_EQ(m.type, Msg::Type::SRCH);
  buf.pop_front();
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("unit-test process_srch, discarded peers")
{
  std::deque<Msg> buf;
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,DELETED,1});
  s.set_edge({3, 0,MST,1});
  s.set_parent_edge({3,0,MST,1});
  s.process({Msg::Type::SRCH,0,3,{3,0}},&buf);
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("unit-test process_srch, mixed peers")
{
  std::deque<Msg> buf;
  GhsState s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,MST,1});
  s.set_edge({3, 0,UNKNOWN,1});
  s.set_edge({4, 0,MST,1});
  s.set_parent_edge({4,0,MST,1});
  s.process({Msg::Type::SRCH,0,4,{4,0}},&buf);

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

TEST_CASE("unit-test process_srch, mixed peers, with parent link")
{
  std::deque<Msg> buf;
  GhsState s(0);
  s.set_partition({3,0});
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,MST,1});
  s.set_edge({3, 0,MST,1});
  s.set_parent_edge( {3,0,MST,1} );
  CHECK_NOTHROW(s.process({Msg::Type::SRCH,0,3,{3,0}},&buf));

  CHECK_EQ(buf.size(),1);

  auto m = buf.front();
  buf.pop_front();
  CHECK_EQ(2, m.to);
  CHECK_EQ(0,m.from);
  CHECK_EQ(m.type,Msg::Type::SRCH);

  CHECK_EQ(buf.size(),0);
  //now from non-parent
  CHECK_THROWS_AS(s.process({Msg::Type::SRCH,0,2,{0,0}},&buf), const std::invalid_argument&);
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

TEST_CASE("unit-test ghs_worst_possible_edge()")
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
  CHECK_EQ(y.metric_val, std::numeric_limits<size_t>::max());
}

TEST_CASE("unit-test process_srch_ret throws when not waiting")
{

  std::deque<Msg> buf;
  GhsState s(0);
  CHECK_THROWS_AS(s.process( Msg{Msg::Type::SRCH_RET,0,1,{}}, &buf), const std::invalid_argument&);
}

TEST_CASE("unit-test process_srch_ret, one peer, no edge found ")
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
  CHECK_EQ(s.mwoe().metric_val, std::numeric_limits<size_t>::max());

  //check the response (leaders either add edges or call for elections)
  CHECK_EQ(buf.size(),1);
  //in this case, we didn't create a good edge, so they assume there's none avail
  CHECK_EQ(buf.front().type, Msg::Type::NOOP);

}

TEST_CASE("unit-test process_srch_ret, one peer, edge found ")
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
  CHECK_EQ(s.mwoe().metric_val, std::numeric_limits<size_t>::max());
  
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

TEST_CASE("unit-test process_srch_ret, one peer, not leader")
{

  std::deque<Msg> buf;
  GhsState s(0);
  //set up one peer, node 1
  s.set_edge({1, 0,MST,1});
  s.set_edge({2, 0,MST,1});
  s.set_parent_edge( {1,0,MST,1} );
  s.set_partition( {1,0} );

  //start it by getting a search from leader to node 0
  s.process({ Msg::Type::SRCH,0,1,{1,0}  }, &buf);
  CHECK_EQ(s.mwoe().root, 0);
  CHECK_EQ(s.mwoe().peer, -1);
  CHECK_EQ(s.mwoe().metric_val, std::numeric_limits<size_t>::max());
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

TEST_CASE("unit-test process_ack_part, happy-path"){
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

TEST_CASE("unit-test process_ack_part, not waiting for anyone"){
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

TEST_CASE("unit-test process_ack_part, no edge"){
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

TEST_CASE("unit-test process_ack_part, waiting, but not for sender"){
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

TEST_CASE("unit-test in_part, happy-path"){
  GhsState s(0);
  std::deque<Msg> buf;
  std::optional<Edge> e;
  //set partition to led by agent 0 with level 2
  s.set_partition({0,2});
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

  //are you, node 0, in partition led by agent 0 with level 3?
  s.process( Msg{Msg::Type::IN_PART,0,1,{0,3}}, &buf);
  //well, I can't say, I'm only level 2
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(s.waiting_count(),0);
  CHECK_EQ(s.delayed_count(),1);
}

TEST_CASE("unit-test process_nack_part, happy-path"){

  GhsState s(0);
  std::deque<Msg> buf;
  Msg m;

  CHECK_NOTHROW(s.set_edge({1,0,UNKNOWN,10}));
  CHECK_NOTHROW(s.set_edge({2,0,UNKNOWN,20}));
  s.set_partition({0,0});

  s.start_round(&buf);
  CHECK_EQ(buf.size(),2);
  buf.pop_front();buf.pop_front();

  CHECK_EQ(s.waiting_count(),2);
  m ={Msg::Type::NACK_PART, 0, 2, {2,0}};
  REQUIRE_NOTHROW(s.process(m,&buf));
  CHECK_EQ(buf.size(),           0);
  CHECK_EQ(s.waiting_count(),    1);
  CHECK_EQ(s.mwoe().metric_val, 20);
  CHECK_EQ(s.mwoe().root,        0);
  CHECK_EQ(s.mwoe().peer,        2);
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument&);
  CHECK_EQ(buf.size(),0);

  m ={Msg::Type::NACK_PART, 0, 1, {1,0}};
  REQUIRE_NOTHROW(s.process(m,&buf));
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
  std::optional<Edge> e;
  CHECK_NOTHROW(e = s.get_edge(s.mwoe().peer));
  CHECK_EQ(e->status,MST);
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument&);
  CHECK_EQ(buf.size(),1);
  
}

TEST_CASE("unit-test process_nack_part, not-leader"){

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
  CHECK_NOTHROW(s.process(Msg{Msg::Type::SRCH,0,3,{3,0}}, &buf));

  CHECK_EQ(buf.size(),2);
  buf.pop_front();
  buf.pop_front();

  CHECK_EQ(s.waiting_count(),2);
  //send msg from 2 --> not in partition
  m ={Msg::Type::NACK_PART, 0, 2, {2,0}};
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
  m ={Msg::Type::NACK_PART, 0, 1, {1,0}};
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

TEST_CASE("unit-test join_us nodes pass")
{

  //JOIN us emits new_sheriff messages, cleverly designed. 
  //2(root) -> 0 -> 1
  auto s = get_state(0,0,0,2,false,false);
  //create a join_us that doesn't involve node or any neighbors
  //note: it's { {edge}, {partition} }
  //aka        {root, peer, leader, level}
  Msg m = {Msg::Type::JOIN_US, 0, 2, {4,5,5,0}};
  std::deque<Msg> buf;
  CHECK_EQ(m.from,2);
  CHECK_EQ(m.to,  0);
  CHECK_THROWS(s.process(m,&buf));
  s.set_partition({5,0});
  CHECK_EQ(s.process(m,&buf), 1);
  CHECK_EQ(buf.size(),        1);
  CHECK_EQ(buf.front().to,    1);
  CHECK_EQ(buf.front().from,  0);
  CHECK_EQ(buf.front().type,  Msg::Type::JOIN_US);
  
}

TEST_CASE("unit-test join_us root relays to peer")
{
  //JOIN us emits new_sheriff messages, cleverly designed. 
  //2(root) -> 0 <-> 1 <- some other root
  auto s = get_state(0,1,0,1,false,false);
  //create a join_us that involves me as root
  //this means I'm in_initiating_partition
  //note: it's { {edge}, {partition} }
  //aka        {root, peer, leader, level}
  REQUIRE_EQ(s.get_id() , 0);
  REQUIRE_EQ(s.get_edge(1)->status , UNKNOWN);
  REQUIRE_EQ(s.get_edge(2)->status , MST);
  Msg m = {Msg::Type::JOIN_US, 0, 2, {1,0,2,0}};
  std::deque<Msg> buf;
  CHECK_EQ(m.from,2);
  CHECK_EQ(m.to,  0);
  CHECK_NOTHROW(s.process(m,&buf));
  CHECK_EQ(s.get_edge(1)->status , MST);
  CHECK_EQ(buf.size(),        1);
  CHECK_EQ(buf.front().to,    1);
  CHECK_EQ(buf.front().from,  0);
  CHECK_EQ(buf.front().type,  Msg::Type::JOIN_US);
  std::optional<Edge> e;
  CHECK_NOTHROW(e = s.get_edge(1));
  //he updated edge
  CHECK_EQ(e->status, MST);
  //check data payload
  CHECK_EQ(buf.front().data[0], 1);//edge to
  CHECK_EQ(buf.front().data[1], 0);//edge from
  CHECK_EQ(buf.front().data[2], 2);//sender's leader
  //level not adjusted
  CHECK_EQ(buf.front().data[3], 0);//sender's level

}

TEST_CASE("unit-test join_us response to higher level")
{
  auto s = get_state(0,1,1,1,false,false);
  //0 is me
  //1 is unknown
  //2 is deleted
  //3 is mst & leader
  //Let's test the case where 1 sends 0 a join_us
  //and we should trigger an obsorb of 1's partition.
  s.set_partition({3,0}); //<-- 0 and 3 are "ahead" w/ level 1
  Msg m = {Msg::Type::JOIN_US,0,1,{0,1,1,1}}; // 1 says to zero "Join my solo partition that I lead with level 0" across edge 0-1
  std::deque<Msg> buf;
  //error: we shoudl never receive a join_ from a partition with higher level!
  CHECK_THROWS_AS(s.process(m,&buf), std::invalid_argument);
  s.set_partition({3,1}); //<-- 0 and 3 are "ahead" w/ level 1
  CHECK_NOTHROW(s.process(m,&buf));
  CHECK_EQ(buf.size(),0);//no msgs to send
  auto e = s.get_edge(1);
  CHECK(e);
  CHECK_EQ(e->status,MST); //we marked as MST
}

TEST_CASE("unit-test join_us response to MST edge")
{
  auto s = get_state(0,1,1,1,false,false);
  //0 is me
  //1 is unknown
  //2 is deleted
  //3 is mst & leader
  //Let's test the case where 1 sends 0 a join_us
  //and we should trigger an obsorb of 1's partition.
  s.set_partition({3,0}); 
  s.set_edge_status(1,MST); //previous one-way join (see prior test case)
  Msg m = {Msg::Type::JOIN_US,0,1,{0,1,1,0}}; // 1 says to zero "Join my solo partition that I lead with level 0" across edge 0-1
  std::deque<Msg> buf;
  CHECK_NOTHROW(s.process(m,&buf));
  CHECK_EQ(buf.size(),0);//No action: we are 0, JOIN_US was from new_leader.
  auto e = s.get_edge(1);
  CHECK(e);
  CHECK_EQ(e->status,MST); //we marked as MST
  CHECK_EQ(s.get_leader_id(), 1);

}

TEST_CASE("unit-test join_us merge")
{
  auto s = get_state(0,1,1,1,false,false);
  //0 is me
  //1 is unknown
  //2 is deleted
  //3 is mst & leader
  //Let's test the case where 1 sends 0 a join_us
  //and we should trigger an obsorb of 1's partition.
  s.set_partition({3,0}); //<-- 0 and 3 are at same level as 0;
  std::deque<Msg> buf;
  Msg m;

  //FIRST, 0 gets the message that the edge 0-1 is the MWOE from partition with leader 3.
  m = {Msg::Type::JOIN_US,0,3,{1,0,3,0}}; // 3 says to zero "add the edge 1<-0 to partition 3 with level 0. 
  CHECK_NOTHROW(s.process(m,&buf));//shouldn't be a problem
  //ok, side effects are:
  //0 sends a message to 1 saying "Join our partition"
  CHECK_EQ(buf.size(), 1);
  CHECK_EQ(buf.front().to, 1);
  CHECK_EQ(buf.front().from, 0);
  CHECK_EQ(buf.front().type, Msg::Type::JOIN_US);
  CHECK_EQ(buf.front().data[0], 1);//<-- edge root
  CHECK_EQ(buf.front().data[1], 0);//<-- edge peer
  CHECK_EQ(buf.front().data[2], 3);//<-- leader 
  CHECK_EQ(buf.front().data[3], 0);//<-- level is not changed during absorb
  CHECK(s.get_edge(1));
  CHECK_EQ(s.get_edge(1)->status, MST);//<-- MST link established

}


TEST_CASE("unit-test join_us merge leader-side")
{
  //essentially the other side of above test
  GhsState s(1);
  //0 is unknown
  //1 is me
  //3 is my leader (not same as 3 above)
  s.set_partition({3,0}); //<-- 1 is alone, level 0
  s.set_edge({3,1,MST,1});
  s.set_edge({0,1,UNKNOWN,1});
  s.set_parent_edge({3,1,MST,1});
  CHECK_EQ(s.get_parent_id(),3); 
  std::deque<Msg> buf;
  Msg m;

  //FIRST, 1 gets the message that the edge 1-0 is the MWOE from partition with leader 2.
  m = {Msg::Type::JOIN_US,1,3,{0,1,3,0}}; // 3 says to 1 "add the edge 1->0 to partition 3 with level 0. 
  CHECK_NOTHROW(s.process(m,&buf));
  //ok, side effects are:
  //1 sends a message to 0 saying "Join our partition"
  CHECK_EQ(buf.size(), 1);
  CHECK_EQ(buf.front().to, 0);
  CHECK_EQ(buf.front().from, 1);
  CHECK_EQ(buf.front().type, Msg::Type::JOIN_US);
  CHECK_EQ(buf.front().data[0], 0);//<-- edge peer (them) 
  CHECK_EQ(buf.front().data[1], 1);//<-- edge root (us)
  CHECK_EQ(buf.front().data[2], 3);//<-- leader 
  CHECK_EQ(buf.front().data[3], 0);//<-- level is not changed during absorb
  //we anticipated their response. A little. 
  CHECK(s.get_edge(0));
  CHECK_EQ(s.get_edge(0)->status, MST);
  buf.pop_front();

  //AND NOW, in a big twist, here comes the join message for 1-0 in the opposite direction
  CHECK_EQ(s.get_id(),1);
  CHECK_EQ(buf.size(), 0);
  m = {Msg::Type::JOIN_US, 1,3, {0,1,3,0}};
  CHECK(s.get_edge(0));
  CHECK_EQ(s.get_edge(0)->status, MST); //<-- still part of the gang
  CHECK_NOTHROW(s.process(m,&buf));
  CHECK(s.get_edge(0));
  CHECK_EQ(s.get_edge(0)->status, MST); //<-- still part of the gang, STILL
  CHECK_EQ(s.get_parent_id(),       1); //This changes because we start_round on ourselves as new leader, which process_srch's on ourselves, setting ourselves as new leader. Desired, but obtuse.
  CHECK_EQ(s.get_partition().level, 1); //level auto-increment b/c we detected merge()
  CHECK_EQ(s.get_partition().leader, 1); //We're leader
  //ok, side effects are:
  //1 sends a message to 0 saying "Join us"
  //AND, it sends a message to SRCH
  CHECK_EQ(buf.size(), 2); //lord only knows why it chooses this order though. 
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);
  CHECK_EQ(buf.front().to, 3);
  CHECK_EQ(buf.back().type, Msg::Type::SRCH); //we have a child to ping
  CHECK_EQ(buf.back().to, 0);

}


TEST_CASE("integration-test two nodes")
{
  GhsState s0(0,{{1,0,UNKNOWN,1}});
  GhsState s1(1,{{0,1,UNKNOWN,2}});
  std::deque<Msg> buf;
  //let's turn the crank and see what happens
  s0.start_round(&buf);
  s1.start_round(&buf);
  CHECK_EQ(buf.size(),2);
  for (int i=0;i<2;i++){
    Msg m = buf.front();
    buf.pop_front();
    CHECK_EQ(m.type,Msg::Type::IN_PART);
    switch(m.to){
      case (0):{ s0.process(m,&buf);break;}
      case (1):{ s1.process(m,&buf);break;}
    }
  }
  CHECK_EQ(buf.size(), 2);
  for (int i=0;i<2;i++){
    Msg m = buf.front();
    buf.pop_front();
    CHECK_EQ(m.type,Msg::Type::NACK_PART);
    switch(m.to){
      case (0):{ s0.process(m,&buf);break;}
      case (1):{ s1.process(m,&buf);break;}
    }
  }
  CHECK_EQ(buf.size(), 2);
  CHECK_EQ(s0.get_partition().level,0); //<--NOT ++ 
  CHECK_EQ(s1.get_partition().level,0); //<--NOT ++ 
  for (int i=0;i<2;i++){
    Msg m = buf.front();
    buf.pop_front();
    CHECK_EQ(m.type,Msg::Type::JOIN_US);
    CHECK_EQ(m.from, m.data[2]); //<-- hey can you join *my* partition with me as leader?
    switch(m.to){
      case (0):{ s0.process(m,&buf);break;}
      case (1):{ s1.process(m,&buf);break;}
    }
  }
  CHECK_EQ(buf.size(), 1); 
  CHECK_EQ(buf.front().type, Msg::Type::SRCH); // leader triggered new round. 
  CHECK_EQ(s0.get_leader_id(),1);
  CHECK_EQ(s0.get_partition().level,1); //<--now ++ 
  CHECK_EQ(s1.get_partition().level,1); //<--now ++ 

}

TEST_CASE("integration-test opposite two nodes")
{
  GhsState s0(0,{{1,0,UNKNOWN,1}});
  GhsState s1(1,{{0,1,UNKNOWN,2}});
  std::deque<Msg> buf;
  //let's turn the crank and see what happens
  //


  //Trigger start
  s1.start_round(&buf);
  s0.start_round(&buf);

  //Nodes sent pings to neighbors
  CHECK_EQ(buf.size(),2); // IN_PART? x2
  for (int i=0;i<2;i++){
    Msg m = buf.front();
    buf.pop_front();
    CHECK_EQ(m.type,Msg::Type::IN_PART);
    switch(m.to){
      case (0):{ s0.process(m,&buf);break;}
      case (1):{ s1.process(m,&buf);break;}
    }
  }

  //Nodes recognized a partition
  CHECK_EQ(buf.size(), 2); // NACK_PART x2
  for (int i=0;i<2;i++){
    Msg m = buf.front();
    buf.pop_front();
    CHECK_EQ(m.type,Msg::Type::NACK_PART);
    switch(m.to){
      case (0):{ s0.process(m,&buf);break;}
      case (1):{ s1.process(m,&buf);break;}
    }
  }

  //Nodes responded with JOIN requests
  CHECK_EQ(buf.size(), 2); // JOIN_US x2
  CHECK_EQ(s0.get_partition().level,0); //<--NOT ++ 
  CHECK_EQ(s1.get_partition().level,0); //<--NOT ++ 
  for (int i=0;i<2;i++){
    Msg m = buf.front();
    buf.pop_front();
    CHECK_EQ(m.type,Msg::Type::JOIN_US);
    CHECK_EQ(m.from, m.data[2]); //<-- hey can you join *my* partition with me as leader?
    switch(m.to){
      case (0):{ s0.process(m,&buf);break;}
      case (1):{ s1.process(m,&buf);break;}
    }
  }

  //Nodes updated graph:
  CHECK_EQ(s0.get_edge(1)->status, MST);
  CHECK_EQ(s1.get_edge(0)->status, MST);

  //nodes understand leader implicitly from prior JOIN_US msgs to be max(0,1)=1
  //... 
  CHECK_EQ(s0.get_leader_id(),1);
  CHECK_EQ(s1.get_leader_id(),1);
  CHECK_EQ(s0.get_partition().level,1); //<--now ++ 
  CHECK_EQ(s1.get_partition().level,1); //<--now ++ also

  //Only one node responded, and it was the leader
  CHECK_EQ(buf.size(), 1); // SRCH from leader only
  CHECK_EQ(buf.front().type, Msg::Type::SRCH);
  CHECK_EQ(buf.front().from , 1);

}

TEST_CASE("sim-test 3 node frenzy")
{

  std::ofstream f("sim-test-3-node-frenzy.msg");

  GhsState states[3]={
    {0},{1},{2}
  };
  for (size_t i=0;i<3;i++){
    for (size_t j=0;j<3;j++){
      if (i!=j){
        //add N^2 edges
        Edge to_add = {i,j,UNKNOWN,(size_t)( (1<<i) + (1<<j))};
        states[j].set_edge(to_add);
      }
    }
  }
  std::deque<Msg> buf;
  for (int i=0;i<3;i++){
    states[i].start_round(&buf);
  }
  int msg_limit = 100;
  int msg_count = 0;
  //here we go:
  //
  while(buf.size()>0 && msg_count++ < msg_limit){
    Msg m = buf.front();

    std::deque<Msg> added;
    buf.pop_front();

    f << "f: "<<states[m.from]<<std::endl;
    f << "t: "<<states[m.to]<<std::endl;
    f << "-  "<<m << std::endl;
    states[m.to].process(m,&added);
    f << "t':"<< states[m.to]<<std::endl;

    for (const auto &m: added){
      f <<"+  "<< m <<std::endl;
      buf.push_back(m);
    }
    f << std::endl;
  }

  CHECK_EQ(buf.size(),0);
  CHECK_LT(msg_count, msg_limit);

  //do we have a single partition?
  for (size_t i=0;i<3;i++){
    CHECK(states[i].is_converged());
    for (size_t j=0;j<3;j++){
      if (i!=j){
        CHECK_EQ(states[i].get_partition().leader, states[j].get_partition().leader);
      }
    }
  }

}
