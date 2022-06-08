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
 * @file tests.cpp
 *
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest/doctest.h"
#include "ghs/ghs.h"
#include "ghs/ghs_printer.h"
#include "ghs/msg_printer.h"
#include <fstream>
#include <vector>

using namespace le::ghs;


template<std::size_t N, std::size_t BUF_SZ>
GhsState<N,BUF_SZ> get_state(agent_t my_id=0, size_t n_unknown=1, size_t n_deleted=0, size_t n_MST=0, bool is_root = true, bool waiting=false)
{
  agent_t id=1;
  std::vector<Edge> edges;
  for (size_t i=0;i<n_unknown;i++, id++){
    metric_t useless_metric = id+i;
    edges.push_back({id,my_id,UNKNOWN, useless_metric});
  }
  for (size_t i=0;i<n_deleted;i++, id++){
    metric_t useless_metric = id+i;
    edges.push_back({id,my_id,DELETED, useless_metric});
  }
  for (size_t i=0;i<n_MST;i++, id++){
    metric_t useless_metric = id+i;
    edges.push_back({id,my_id,MST, useless_metric});
  }

  GhsState<N,BUF_SZ> ghs(0,{},0);

  if (!is_root){
    metric_t useless_metric =(metric_t) id;
    //just set the last MST link as the one to the root
    edges.push_back( {id-1,0,MST_PARENT,useless_metric} );
  }
  
  ghs = GhsState<N,BUF_SZ>(my_id,edges.data(),edges.size());

  REQUIRE_EQ(ghs.get_n_peers(),id-1);

  if (!is_root){
    REQUIRE_EQ(id-1,ghs.get_parent_id());
  }

  for (int i=1;i<id;i++){
    REQUIRE(ghs.has_edge(i));
  }

  return ghs;
}

TEST_CASE("unit-test get_state"){
  auto s = get_state<32,32>(0,1,1,1,false,false);
  (void)s;
}

TEST_CASE("unit-test checked_index_of")
{
  auto s = get_state<32,32>(0,1,1,1,false,false);
  size_t idx;
  CHECK_EQ(OK, s.checked_index_of(1,idx));
  CHECK_EQ(OK, s.checked_index_of(2,idx));
  CHECK_EQ(OK, s.checked_index_of(3,idx));
  CHECK_EQ(NO_SUCH_PEER, s.checked_index_of(4,idx));
  CHECK_EQ(NO_SUCH_PEER, s.checked_index_of(-1,idx));
  CHECK_EQ(IMPL_REQ_PEER_MY_ID, s.checked_index_of(0,idx));
}

TEST_CASE("unit-test typecast")
{

  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0,{},0);
  msg::Data d;
  d.srch = SrchPayload{0,0};
  size_t sent;
  CHECK_EQ(OK, s.typecast(UNKNOWN, msg::Type::SRCH, d, buf, sent));
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop(); }

  s = get_state<4,32>(0,1);
  msg::Data pld;
  pld.srch={0,0};
  CHECK_EQ(OK,s.typecast(UNKNOWN, msg::Type::SRCH, pld, buf, sent));
  //got one
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  Msg buf_front;

  CHECK( OK==( buf.front(buf_front) ) );
  CHECK_EQ(buf_front.from(), 0);
  CHECK_EQ(buf_front.to(), 1);
  CHECK_EQ(buf_front.type(), msg::Type::SRCH);

  while (buf.size()>0){ buf.pop(); }

  //send to unknown, but have MST link
  s = get_state<4,32>(0,1,0,1,false);
  //look for unknown
  pld.srch={0,0};
  CHECK_EQ(OK,s.typecast(UNKNOWN, msg::Type::SRCH, pld, buf, sent));
  //only one still, right?
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK( OK==( buf.front(buf_front) ) );
  CHECK_EQ(buf_front.from(), 0);
  CHECK_EQ(buf_front.to(), 1); //<-- not 2!
  CHECK_EQ(buf_front.type(), msg::Type::SRCH);

  while (buf.size()>0){ buf.pop(); }

  //broadcast as root
  s = get_state<4,32>(0,1,0,1,true);
  pld.srch={0,0};
  CHECK_EQ(s.mst_broadcast(msg::Type::SRCH, pld , buf, sent), OK);
  //Now got one here too
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  CHECK( OK==( buf.front(buf_front) ) );
  CHECK_EQ(buf_front.from(), 0);
  CHECK_EQ(buf_front.to(), 2);
  CHECK_EQ(buf_front.type(), msg::Type::SRCH);

}

TEST_CASE("unit-test mst_broadcast")
{

  StaticQueue<Msg,32> buf;
  auto s = get_state<4,32>(0,0,0,0);
  msg::Data pld;
  pld.srch={0,0};
  size_t sent=0;
  CHECK_EQ(OK,s.mst_broadcast(msg::Type::SRCH, pld,  buf, sent));
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop(); }

  s = get_state<4,32>(0,1,0,0);
  pld.srch={0,0};
  CHECK_EQ(OK, s.mst_broadcast(msg::Type::SRCH, pld, buf, sent));
  //nobody to send to, still
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  //we're a leaf node
  s = get_state<4,32>(0,1,0,1,false);
  pld.srch={0,0};
  CHECK_EQ(OK, s.mst_broadcast(msg::Type::SRCH, pld, buf, sent));
  //nobody to send to, still, right!?
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  //we're a link b/w leaf and root
  s = get_state<4,32>(0,1,0,2,false);
  pld.srch={0,0};
  CHECK_EQ(OK,s.mst_broadcast(msg::Type::SRCH, pld, buf, sent));
  //Now got one
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);

  Msg buf_front;
  CHECK( OK==( buf.front(buf_front) ) );
  CHECK_EQ(buf_front.from(), 0);
  CHECK_EQ(buf_front.to(), 2);
  CHECK_EQ(buf_front.type(), msg::Type::SRCH);

}

TEST_CASE("unit-test mst_convergecast")
{

  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0,{},0);
  size_t sent;
  msg::Data pld;
  pld.srch={0,0};
  CHECK_EQ(OK,s.mst_convergecast(msg::Type::SRCH, pld,  buf, sent));
  //nobody to send to
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop(); }

  s = get_state<4,32>(0,1,0,0,true);
  pld.srch={0,0};
  CHECK_EQ(OK,s.mst_convergecast(msg::Type::SRCH, pld, buf, sent));
  //nobody to send to, still
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop(); }

  //Still can't send to children MST
  s = get_state<4,32>(0,1,0,1,true);
  pld.srch={0,0};
  CHECK_EQ(OK,s.mst_convergecast(msg::Type::SRCH, pld,  buf, sent));
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(sent,0);

  while (buf.size()>0){ buf.pop(); }

  //Still can't send to children MST
  s = get_state<4,32>(0,1,0,1,false);
  pld.srch={0,0};
  CHECK_EQ(OK, s.mst_convergecast(msg::Type::SRCH, pld, buf, sent));
  //Only to parents!
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(sent,1);
  Msg buf_front;
  CHECK( OK==( buf.front(buf_front) ) );
  CHECK_EQ(buf_front.from(), 0);
  CHECK_EQ(buf_front.to(), 2);
  CHECK_EQ(buf_front.type(), msg::Type::SRCH);

}

/*
TEST_CASE("unit-test start_round() on leader, unknown peers")
{
  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,UNKNOWN,1});
  s.set_edge({2, 0,UNKNOWN,1});
  size_t sz;
  CHECK_EQ(OK, s.start_round(buf, sz));
  CHECK_EQ(buf.size(),2);
  while(buf.size()>0){ 
    //auto m = buf.front();
    Msg buf_front;
    CHECK( OK==( buf.front(buf_front) ) );
    //CHECK_EQ(m.type(), msg::Type::IN_PART);
    CHECK_EQ(buf_front.type(), msg::Type::IN_PART);
    buf.pop();
  }
}

TEST_CASE("unit-test start_round() on leader, mst peers")
{
  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,MST,1});
  s.set_edge({2, 0,MST,1});
  size_t sz;
  s.start_round(buf, sz);

  CHECK_EQ(buf.size(),2);
  Msg m;
  CHECK( OK==( buf.front(m) ) );
  CHECK_EQ(m.type(), msg::Type::SRCH);
  buf.pop();

  CHECK( OK==( buf.front(m) ) );
  CHECK_EQ(m.type(), msg::Type::SRCH);
  buf.pop();
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("unit-test start_round() on leader, discarded peers")
{
  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  REQUIRE_EQ(buf.size(),0);
  CHECK_EQ(OK, s.set_edge({1, 0,DELETED,1}));
  CHECK_EQ(OK, s.set_edge({2, 0,DELETED,1}));
  size_t sz;
  CHECK_EQ(OK, s.start_round(buf, sz));
  //do they report no MWOE to parent? No, no parent
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("unit-test start_round() on leader, mixed peers")
{
  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  REQUIRE_EQ(buf.size(),0);
  CHECK_EQ(OK, s.set_edge({1, 0,DELETED,1}));
  CHECK_EQ(OK, s.set_edge({2, 0,MST,1}));
  CHECK_EQ(OK, s.set_edge({3, 0,UNKNOWN,1}));
  size_t sz;
  CHECK_EQ(OK, s.start_round(buf, sz));

  CHECK_EQ(buf.size(),2);

  //auto m = buf.front();
  Msg m;
  CHECK( OK==( buf.front(m) ) );
  buf.pop();
  CHECK_EQ(2, m.to());
  CHECK_EQ(0,m.from());
  CHECK_EQ(m.type(),msg::Type::SRCH);

  //m = buf.front();
  //Msg m;
  CHECK( OK==( buf.front(m) ) );
  buf.pop();
  CHECK_EQ(3, m.to());
  CHECK_EQ(0, m.from());
  CHECK_EQ(m.type(),msg::Type::IN_PART);

  CHECK_EQ(buf.size(),0);
}

TEST_CASE("unit-test start_round() on non-leader")
{
  StaticQueue<Msg,32> buf;
  //set id to 0, and 4 total nodes
  GhsState<4,32> s(0);
  //set leader to 1, level to 0 (ignored)
  CHECK_EQ(OK, s.set_leader_id(1));
  CHECK_EQ(OK,  s.set_level(0));

  REQUIRE_EQ(buf.size(),0);
  CHECK_EQ(OK, s.set_edge({1, 0,DELETED,1}));
  CHECK_EQ(OK, s.set_edge({2, 0,MST,1}));
  CHECK_EQ(OK, s.set_edge({3, 0,UNKNOWN,1}));
  size_t sz;
  CHECK_EQ(OK, s.start_round(buf, sz));

  //better have done nothing!
  CHECK_EQ(buf.size(),0);

}

TEST_CASE("unit-test process_srch() checks recipient"){

  StaticQueue<Msg,32> buf;
  //set id to 0, and 4 total nodes
  GhsState<4,32> s(0);
  //set leader to 1, level to 0 
  s.set_leader_id(1);
  s.set_level(0);
  s.set_edge({1,0,MST,1});
  //from rando
  Msg m(0,2,SrchPayload{1,0});
  //for documentation:
  CHECK_EQ(m.from(), 2);
  size_t sz=111;
  CHECK_EQ(PROCESS_NO_EDGE_FOUND, s.process( m, buf, sz));
  CHECK_EQ(sz,111);//didn't touch it!
  //from leader is ok
  Msg from_one = Msg(m.to(),1,m.data().srch);
  CHECK_EQ(OK, s.process( from_one , buf, sz));
  //from self not allowed
  Msg from_self= Msg(m.to(),0,m.data().srch);
  CHECK_EQ(PROCESS_SELFMSG, s.process( from_self, buf, sz));
    
}

TEST_CASE("unit-test process_srch, unknown peers")
{
  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  REQUIRE_EQ(buf.size(),0);
  CHECK_EQ(OK, s.set_edge({1, 0,UNKNOWN,1}));
  CHECK_EQ(OK,   s.set_edge({2, 0,UNKNOWN,1}));
  size_t sz=111;
  CHECK_EQ(OK,   s.start_round(buf, sz));
  CHECK_EQ(buf.size(),2);
  CHECK_EQ(buf.size(),sz);
  while(buf.size()>0){ 
    //auto m = buf.front();

    Msg m;
    CHECK( OK==( buf.front(m) ) );
    CHECK_EQ(m.type(), msg::Type::IN_PART);
    buf.pop();
  }
}

TEST_CASE("unit-test process_srch,  mst peers")
{
  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  REQUIRE_EQ(buf.size(),0);
  CHECK_EQ(OK, s.set_edge({1, 0,MST,1}));
  CHECK_EQ(OK, s.set_edge({2, 0,MST,1}));
  CHECK_EQ(OK, s.set_edge({3, 0,MST,1}));
  CHECK_EQ(OK, s.set_parent_id(3));
  Msg m = Msg(0,3,SrchPayload{3,0});
  size_t sz=100;
  CHECK_EQ(OK, s.process(m,buf, sz));

  CHECK_EQ(buf.size(),2);
  CHECK_EQ(buf.size(),sz);
  //auto m = buf.front();
  CHECK( OK==( buf.front(m) ) );
  CHECK_EQ(m.type(), msg::Type::SRCH);
  buf.pop();
  //m = buf.front();
  //Msg m;
  CHECK( OK==( buf.front(m) ) );
  CHECK_EQ(m.type(), msg::Type::SRCH);
  buf.pop();
  CHECK_EQ(buf.size(),0);
}

TEST_CASE("unit-test process_srch, discarded peers")
{
  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,DELETED,1});
  s.set_edge({3, 0,MST,1});
  s.set_parent_id(3);
  Msg m = Msg(0,3,SrchPayload{3,0} );
  size_t sz;
  s.process(m,buf, sz);
  //they should report no MWOE to parent. 
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(buf.size(),sz);
}

TEST_CASE("unit-test process_srch, mixed peers")
{
  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,MST,1});
  s.set_edge({3, 0,UNKNOWN,1});
  s.set_edge({4, 0,MST,1});
  s.set_parent_id(4);
  Msg m = Msg(0,4,SrchPayload{4,0} );
  size_t sz;
  s.process(m,buf, sz);

  CHECK_EQ(buf.size(),2);
  CHECK_EQ(buf.size(),sz);

  //auto m = buf.front();
  CHECK( OK==( buf.front(m) ) );
  buf.pop();
  CHECK_EQ(2, m.to());
  CHECK_EQ(0,m.from());
  CHECK_EQ(m.type(),msg::Type::SRCH);

  //m = buf.front();
  //Msg m;
  CHECK( OK==( buf.front(m) ) );
  buf.pop();
  CHECK_EQ(3, m.to());
  CHECK_EQ(0, m.from());
  CHECK_EQ(m.type(),msg::Type::IN_PART);

  CHECK_EQ(buf.size(),0);
}

TEST_CASE("unit-test process_srch, mixed peers, with parent link")
{
  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  s.set_leader_id(3);
  s.set_level(0);
  REQUIRE_EQ(buf.size(),0);
  s.set_edge({1, 0,DELETED,1});
  s.set_edge({2, 0,MST,1});
  s.set_edge({3, 0,MST,1});
  s.set_parent_id(3);
  Msg m = Msg(0,3,SrchPayload{3,0} );
  size_t sz;
  CHECK_EQ(OK,s.process(m,buf, sz));
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(buf.size(),sz);

  //auto m = buf.front();
  //
  CHECK( OK==( buf.front(m) ) );
  buf.pop();
  CHECK_EQ(2, m.to());
  CHECK_EQ(0,m.from());
  CHECK_EQ(m.type(),msg::Type::SRCH);

  CHECK_EQ(buf.size(),0);
  //now from non-parent, and btw we're busy
  m = Msg(0,2,SrchPayload{0,0} );
  CHECK_EQ(SRCH_STILL_WAITING, s.process(m,buf,sz));
}

TEST_CASE("Guard against Edge refactoring"){
  Edge y = Edge{4,1,UNKNOWN, 1000};
  CHECK(y.peer == 4);
  CHECK(y.root== 1);
  CHECK(y.metric_val== 1000);
}

TEST_CASE("unit-test worst_edge()")
{
  Edge edge = worst_edge();
  CHECK(edge.peer == NO_AGENT);
  CHECK(edge.root== NO_AGENT);
  CHECK(edge.status== UNKNOWN);
  CHECK(edge.metric_val == WORST_METRIC);
  Edge y = Edge{4,1,UNKNOWN, 1000};
  CHECK(y.peer == 4);
  CHECK(y.root== 1);
  CHECK(y.metric_val== 1000);
  y = edge;
  CHECK_EQ(y.metric_val,WORST_METRIC );
}

TEST_CASE("unit-test process throws with no edge")
{

  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  size_t sz;
  Msg m = Msg(0,1, SrchRetPayload{});
  CHECK_EQ(PROCESS_NO_EDGE_FOUND, s.process(m,buf,sz));
}

TEST_CASE("unit-test process_srch_ret")
{

  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  //set up one peer, node 1
  s.set_edge({1, 0,MST,1});

  //start it
  size_t sz;
  s.start_round(buf, sz);

  CHECK_EQ(buf.size(),1); // SRCH
  CHECK_EQ(buf.size(),sz); // SRCH
  Msg m;
  CHECK( OK==( buf.front(m) ) );
  //CHECK_EQ(buf.front().type(), msg::Type::SRCH);
  CHECK_EQ(m.type(),msg::Type::SRCH);
  buf.pop();
  CHECK_EQ(s.waiting_count(),1);//the MST and UNKNOWN edges
  //pretend node 1 returned a bad edge
  auto bad_edge = worst_edge();

  //send a return message 
  auto srch_ret_msg =Msg(0,1,SrchRetPayload{1,2,bad_edge.metric_val});
  CHECK_EQ(OK,s.process( srch_ret_msg, buf, sz));
  CHECK_EQ(s.waiting_count(),0);
  //did not accept their edge
  CHECK_EQ(s.mwoe().root, 0);
  CHECK_EQ(s.mwoe().peer, -1);
  CHECK_EQ(s.mwoe().metric_val, WORST_METRIC);

  //check the response (leaders either add edges or call for elections)
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(buf.size(),sz);
  //in this case, we didn't create a good edge, so they assume there's none avail
  //CHECK_EQ(buf.front().type(), msg::Type::NOOP);

  //Msg m;
  CHECK( OK==( buf.front(m) ) );
  CHECK_EQ(m.type(), msg::Type::NOOP);


}

TEST_CASE("unit-test process_srch_ret, one peer, edge found ")
{

  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  //set up one peer, node 1
  s.set_edge({1, 0,MST,1});
  s.set_leader_id(0);
  s.set_level(0);

  //start it
  size_t sz;
  s.start_round(buf, sz);

  CHECK_EQ(buf.size(),1); // SRCH
  CHECK_EQ(buf.size(),sz); // SRCH
  //CHECK_EQ(buf.front().type(), msg::Type::SRCH);
  Msg m;
  CHECK( OK==( buf.front(m) ) );
  CHECK_EQ(m.type(),msg::Type::SRCH);
  CHECK( OK==(buf.pop()));
  CHECK_EQ(s.waiting_count(),1);
  CHECK_EQ(s.mwoe().root, 0);
  CHECK_EQ(s.mwoe().peer, -1);
  CHECK_EQ(s.mwoe().metric_val, WORST_METRIC);
  
  //pretend node 1 returned a good edge (1-->2, wt=0)
  //send a return message 
  auto srch_ret_msg =Msg(0,1,SrchRetPayload{2,1,1});

  CHECK_EQ(OK,s.process( srch_ret_msg, buf, sz));

  //that should be the repsponse it was looking for
  CHECK_EQ(s.waiting_count(),0);
  //accepted their edge
  CHECK_EQ(s.mwoe().root, 1);
  CHECK_EQ(s.mwoe().peer, 2);
  CHECK_EQ(s.mwoe().metric_val, 1);

  //check the response (leaders either add edges or call for elections)
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(buf.size(),sz);
  //in this case, we have a candidate, that should join_us
  //Msg m;
  CHECK( OK==( buf.front(m) ) );
  //CHECK_EQ(buf.front().type(), msg::Type::JOIN_US);
  CHECK_EQ(m.type(), msg::Type::JOIN_US);

}

TEST_CASE("unit-test process_srch_ret, one peer, not leader")
{

  StaticQueue<Msg,32> buf;
  GhsState<4,32> s(0);
  //set up one peer, node 1
  s.set_edge({1, 0,MST,1});
  s.set_edge({2, 0,MST,1});
  s.set_parent_id(1);
  s.set_leader_id(1); 
  s.set_level(0);

  //start it by getting a search from leader to node 0
  Msg m =Msg(0,1,SrchPayload{1,0});
  size_t sz;
  s.process( m, buf, sz);
  CHECK_EQ(s.mwoe().root, 0);
  CHECK_EQ(s.mwoe().peer, -1);
  CHECK_EQ(s.mwoe().metric_val, WORST_METRIC);
  CHECK_EQ(buf.size(),1); // SRCH to 2
  CHECK_EQ(buf.size(),sz); 

  Msg buf_front;
  CHECK( OK==( buf.front(buf_front) ) );
  CHECK_EQ(buf_front.type(), msg::Type::SRCH);
  CHECK_EQ(buf_front.to(), 2);
  buf.pop();
  CHECK_EQ(s.waiting_count(),1);//waiting on node 2
  //pretend node 2 returned a good edge (from 2 to 3 with wt 0) back to node 0
  
  auto srch_ret_msg =Msg(0,2,SrchRetPayload{3,2,1});

  CHECK_EQ(OK,s.process( srch_ret_msg, buf, sz));
  CHECK_EQ(s.waiting_count(),0);
  CHECK_EQ(s.mwoe().root, 2);
  CHECK_EQ(s.mwoe().peer, 3);
  CHECK_EQ(s.mwoe().metric_val, 1);

  //check the response (leaders either add edges or call for elections)
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(buf.size(),sz);
  //in this case, we have to tell parent
  CHECK ( OK==( buf.front(buf_front) ) );
  CHECK_EQ(buf_front.type(), msg::Type::SRCH_RET);
  CHECK_EQ(buf_front.to(), 1);

}

TEST_CASE("unit-test process_ack_part, happy-path"){
  GhsState<4,32> s(0);
  StaticQueue<Msg,32> buf;
  Edge e;

  //create edge to 1
  Edge e1 = {1,0,UNKNOWN,10};
  CHECK_EQ(1, e1.peer);
  CHECK_EQ(0, e1.root);
  CHECK_EQ(OK,s.set_edge( e1 ));
  CHECK_EQ(OK,s.get_edge(1,e));
  CHECK_EQ(e.status, UNKNOWN);
  size_t sz;
  s.start_round(buf, sz);
  CHECK_EQ(1,s.waiting_count());
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(buf.size(),sz);
  buf.pop(); 

  Msg m =Msg(0,1,AckPartPayload{}); 
  CHECK_EQ(m.from(),1);//<-- code should modify using "from" field
  CHECK_EQ(OK,s.process(m,buf,sz));
  CHECK_EQ(OK, s.get_edge(1,e));
  CHECK_EQ(e.status, DELETED);
}

TEST_CASE("unit-test process_ack_part, not waiting for anyone"){
  GhsState<4,32> s(0);
  StaticQueue<Msg,32> buf;

  //create edge to 1
  Edge e1 = {1,0,UNKNOWN,10};
  CHECK_EQ(1, e1.peer);
  CHECK_EQ(0, e1.root);
  CHECK_EQ(OK,s.set_edge( e1 ));
  Edge e;
  CHECK_EQ(OK, s.get_edge(1,e));
  CHECK_EQ(e.status, UNKNOWN);

  CHECK_EQ(0,s.waiting_count());
  
  Msg m =Msg(0,1,AckPartPayload{});
  CHECK_EQ(m.from(),1);//<-- code should modify using "from" field
  size_t sz;
  CHECK_EQ(ACK_NOT_WAITING, s.process(m,buf,sz));
  Edge tmp;
  CHECK_EQ(OK, s.get_edge(1,tmp));
  CHECK_EQ(tmp.status, UNKNOWN); //<--unmodified!
}

TEST_CASE("unit-test process_ack_part, no edge"){
  GhsState<4,32> s(0);
  StaticQueue<Msg,32> buf;
  Edge e;

  //create edge to 1
  Edge e1 = {1,0,UNKNOWN,10};
  CHECK_EQ(1, e1.peer);
  CHECK_EQ(0, e1.root);
  CHECK( !s.has_edge(1) );
  s.add_edge(e1);
  CHECK( s.has_edge(1) );
  CHECK_EQ(OK, s.get_edge(1,e));
  CHECK_EQ(0,s.waiting_count());

  Msg m =Msg(0,1,AckPartPayload{});
  CHECK_EQ(m.from(),1);//<-- code should modify using "from" field
  size_t sz;
  CHECK_EQ(ACK_NOT_WAITING,s.process(m,buf, sz));
}

TEST_CASE("unit-test process_ack_part, waiting, but not for sender"){
  GhsState<4,32> s(0);
  StaticQueue<Msg,32> buf;
  Edge e;

  //create edge to 1
  Edge e1 = {1,0,UNKNOWN,10};
  Edge e2 = {2,0,UNKNOWN,10};
  CHECK_EQ(OK,s.set_edge( e1 ));
  CHECK_EQ(OK,s.set_edge( e2 ));
  size_t sz;
  s.start_round(buf, sz);
  CHECK_EQ(2,s.waiting_count());
  CHECK_EQ(buf.size(),2);
  buf.pop(); 
  buf.pop(); 

  Msg m =Msg(0,2,AckPartPayload{}); 
  CHECK_EQ(OK, s.get_edge(2,e));
  CHECK_EQ(m.from(),2);   //<-- code should modify using "from" field
  CHECK_EQ(OK,s.process(m,buf, sz));
  CHECK_EQ(1,s.waiting_count()); //<-- got data we need from 2
  //send msg again:
  CHECK_EQ(ACK_NOT_WAITING,s.process(m,buf, sz));
}

TEST_CASE("unit-test in_part, happy-path"){
  GhsState<4,32> s(0);
  StaticQueue<Msg,32> buf;
  //set partition to led by agent 0 with level 2
  s.set_leader_id(0);
  s.set_level(2);
  //are you, node 0, in partition led by agent 1 with level 2? 
  s.add_edge( {1,0,UNKNOWN,999} );
  Msg m =Msg(0,1,InPartPayload{1,2});
  size_t sz;
  CHECK_EQ(OK, s.process( m, buf, sz));
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(s.waiting_count(),0);
  //no, I am not
  CHECK (OK==(buf.front(m)));
  CHECK_EQ(m.type(), msg::Type::NACK_PART);
  buf.pop();

  //are you, node 0,  in partition led by agent 0 with level 2? 
  m =Msg(0,1,InPartPayload{0,2});
  CHECK_EQ(OK, s.process( m, buf, sz));
  CHECK_EQ(buf.size(),1);
  CHECK_EQ(buf.size(),sz);
  CHECK_EQ(s.waiting_count(),0);
  //yes, I am
  CHECK (OK==(buf.front(m)));
  CHECK_EQ(m.type(), msg::Type::ACK_PART);
  buf.pop();

  //are you, node 0, in partition led by agent 0 with level 3?
  m =Msg(0,1,InPartPayload{0,3});
  CHECK_EQ(OK, s.process( m, buf, sz));
  //well, I can't say, I'm only level 2
  CHECK_EQ(buf.size(),0);
  CHECK_EQ(buf.size(),sz);
  CHECK_EQ(s.waiting_count(),0);
  CHECK_EQ(s.delayed_count(),1);
}

TEST_CASE("unit-test process_nack_part, happy-path"){

  GhsState<4,32> s(0);
  StaticQueue<Msg,32> buf;
  Msg m;

  CHECK_EQ(OK,s.set_edge({1,0,UNKNOWN,10}));
  CHECK_EQ(OK,s.set_edge({2,0,UNKNOWN,20}));
  s.set_leader_id(0);
  s.set_level(0);

  size_t sz;
  s.start_round(buf, sz);
  CHECK_EQ(buf.size(),2);
  buf.pop();buf.pop();

  CHECK_EQ(s.waiting_count(),2);
  m =Msg(0,2,NackPartPayload{}); 
  REQUIRE_EQ(OK,s.process(m,buf, sz));
  CHECK_EQ(buf.size(),           0);
  CHECK_EQ(s.waiting_count(),    1);
  CHECK_EQ(s.mwoe().metric_val, 20);
  CHECK_EQ(s.mwoe().root,        0);
  CHECK_EQ(s.mwoe().peer,        2);
  //can't do twice
  CHECK_EQ(ACK_NOT_WAITING, s.process(m,buf, sz));
  CHECK_EQ(buf.size(),0);


  m =Msg(0,1,NackPartPayload{}); 
  REQUIRE_EQ(OK,s.process(m,buf, sz));
  CHECK_EQ(buf.size(),           1);
  CHECK_EQ(s.waiting_count(),    0);
  CHECK_EQ(s.mwoe().metric_val,  10);
  CHECK_EQ(s.mwoe().root,        0);
  CHECK_EQ(s.mwoe().peer,        1);

  //response should be to tell the other guy we're joining up
  Msg buf_front;
  CHECK( OK==( buf.front(buf_front) ) );
  CHECK_EQ(buf_front.type(),  msg::Type::JOIN_US);
  CHECK_EQ(buf_front.to(),    s.mwoe().peer);
  CHECK_EQ(buf_front.from(),  s.mwoe().root);
  CHECK_EQ(buf_front.from(),  0);
  Edge e;
  CHECK_EQ(OK,s.get_edge(s.mwoe().peer,e));
  CHECK_EQ(e.status,MST);
  //again, twice doesn't fly
  CHECK_EQ(ACK_NOT_WAITING,s.process(m,buf, sz));
  CHECK_EQ(buf.size(),1);
  
}

TEST_CASE("unit-test process_nack_part, not-leader"){

  GhsState<4,32> s(0);
  StaticQueue<Msg,32> buf;
  Msg m;

  //add two outgoing unkonwn edges
  CHECK_EQ(OK,s.set_edge({1,0,UNKNOWN,10}));
  CHECK_EQ(OK,s.set_edge({2,0,UNKNOWN,20}));
  //add leader
  CHECK_EQ(OK,s.set_edge({3,0,MST,20}));
  CHECK_EQ(OK,s.set_parent_id(3));
  CHECK_EQ(OK,s.set_leader_id(3));
  CHECK_EQ(OK,s.set_level(0));


  //send the start message
  m =Msg(0,3,SrchPayload{3,0});
  size_t sz;
  CHECK_EQ(OK,s.process(m, buf, sz));

  CHECK_EQ(buf.size(),2);
  buf.pop();
  buf.pop();

  CHECK_EQ(s.waiting_count(),2);
  //send msg from 2 --> not in partition
  m =Msg(0,2,NackPartPayload{});
  s.process(m,buf, sz);
  CHECK_EQ(buf.size(),           0);
  CHECK_EQ(s.waiting_count(),    1);
  CHECK_EQ(s.mwoe().metric_val, 20);
  CHECK_EQ(s.mwoe().root,        0);
  CHECK_EQ(s.mwoe().peer,        2);
  //check error condition
  CHECK_EQ(ACK_NOT_WAITING,s.process(m,buf, sz));
  CHECK_EQ(buf.size(),0);

  //send message from 1 --> not in partition
  m =Msg(0,1,NackPartPayload{});
  CHECK_EQ(OK,s.process(m,buf, sz));
  CHECK_EQ(ACK_NOT_WAITING,s.process(m,buf, sz));
  CHECK_EQ(buf.size(),           1);
  CHECK_EQ(buf.size(),          sz);
  CHECK_EQ(s.waiting_count(),    0);
  CHECK_EQ(s.mwoe().metric_val,  10);
  CHECK_EQ(s.mwoe().root,        0);
  CHECK_EQ(s.mwoe().peer,        1);

  //since we got both respones, but have leader
  //response should be to tell the boss
  Msg buf_front;
  CHECK( OK== ( buf.front( buf_front) ) );
  CHECK_EQ(buf_front.type(),  msg::Type::SRCH_RET);
  CHECK_EQ(buf_front.to(),    s.get_leader_id());
  CHECK_EQ(buf_front.from(),  0);
  //should not add edge yet
  Edge e; 
  CHECK_EQ(OK,s.get_edge(s.mwoe().peer,e));
  CHECK_EQ(e.status, UNKNOWN);
  CHECK_EQ(buf.size(),1);

}

TEST_CASE("unit-test join_us nodes pass")
{

  //JOIN us emits new_sheriff messages, cleverly designed. 
  //2(root) -> 0 -> 1
  auto s = get_state<4,32>(0,0,0,2,false,false);
  //create a join_us that doesn't involve node or any neighbors
  //note: it's { {edge}, {partition} }
  //aka        {root, peer, leader, level}
  Msg m;
  m =Msg(0,2,JoinUsPayload{4,5,5,0});
  
  StaticQueue<Msg,32> buf;
  CHECK_EQ(m.from(),2);
  CHECK_EQ(m.to(),  0);
  size_t sz;
  CHECK_EQ(JOIN_BAD_LEADER, s.process(m,buf, sz));
  s.set_leader_id(5);
  s.set_level(0);
  CHECK_EQ(s.process(m,buf, sz), OK);
  CHECK_EQ(buf.size(),        1);
  CHECK_EQ(buf.size(),       sz);
  Msg buf_front;
  CHECK(OK==(buf.front(buf_front)));
  CHECK_EQ(buf_front.to(),    1);
  CHECK_EQ(buf_front.from(),  0);
  CHECK_EQ(buf_front.type(),  msg::Type::JOIN_US);
  
}

TEST_CASE("unit-test join_us root relays to peer")
{
  //JOIN us emits new_sheriff messages, cleverly designed. 
  //2(root) -> 0 <-> 1 <- some other root
  auto s = get_state<3,32>(0,1,0,1,false,false);
  //create a join_us that involves me as root
  //this means I'm in_initiating_partition
  //note: it's { {edge}, {partition} }
  //aka        {root, peer, leader, level}
  REQUIRE_EQ(s.get_id() , 0);
  REQUIRE(s.has_edge(1));
  Edge e;
  s.get_edge(1,e);
  REQUIRE_EQ(e.status, UNKNOWN);
  REQUIRE(s.has_edge(2));
  s.get_edge(2,e);
  REQUIRE_EQ(e.status, MST);
  Msg m =Msg(0,2,JoinUsPayload{1,0,2,0});
  StaticQueue<Msg,32> buf;
  CHECK_EQ(m.from(),2);
  CHECK_EQ(m.to(),  0);
  size_t sz;
  CHECK_EQ(OK,s.process(m,buf, sz));
  s.get_edge(1,e);
  CHECK_EQ(e.status , MST);
  CHECK_EQ(buf.size(),        1);

  Msg buf_front;
  CHECK(OK==(buf.front(buf_front)));
  CHECK_EQ(buf_front.to(),    1);
  CHECK_EQ(buf_front.from(),  0);
  CHECK_EQ(buf_front.type(),  msg::Type::JOIN_US);
  CHECK_EQ(OK,s.get_edge(1,e));
  //he updated edge
  CHECK_EQ(e.status, MST);
  //check data payload
  CHECK(OK==(buf.front(buf_front)));
  CHECK_EQ(buf_front.data().join_us.join_peer, 1);//edge to
  CHECK_EQ(buf_front.data().join_us.join_root, 0);//edge from
  CHECK_EQ(buf_front.data().join_us.proposed_leader, 2);//sender's leader
  //level not adjusted
  CHECK_EQ(buf_front.data().join_us.proposed_level, 0);//sender's level

}

TEST_CASE("unit-test join_us response to higher level")
{
  auto s = get_state<4,32>(0,1,1,1,false,false);
  //0 is me
  //1 is unknown
  //2 is deleted
  //3 is mst & leader
  //Let's test the case where 1 sends 0 a join_us
  //and we should trigger an obsorb of 1's partition.
  s.set_leader_id(3);
  s.set_level(0);
  Msg m_one =Msg(0,1,JoinUsPayload{0,1,1,1});// 1 says to zero "Join my solo partition that I lead with level 1" across edge 0-1
  StaticQueue<Msg,32> buf;
  //error: we shoudl never receive a join_ from a this person with this level
  size_t sz;
  //we detect that they should not have recieved our response to reply to, yet, since they are higher level
  CHECK_EQ(JOIN_UNEXPECTED_REPLY,s.process(m_one,buf, sz));
  //let's assume we are at their level
  s.set_level(1);
  Edge e;
  CHECK_EQ(OK, s.get_edge(m_one.from(), e));
  CHECK_EQ(OK, s.process(m_one,buf, sz));
  CHECK_EQ(buf.size(),0);//no msgs to send
  Edge tmp;
  CHECK_EQ(OK,s.get_edge(1,tmp));
  CHECK_EQ(tmp.status,MST); //we marked as MST
}

TEST_CASE("unit-test join_us response to MST edge")
{
  auto s = get_state<4,32>(0,1,1,1,false,false);
  //0 is me
  //1 is unknown
  //2 is deleted
  //3 is mst & leader
  //Let's test the case where 1 sends 0 a join_us
  //and we should trigger an obsorb of 1's partition.
  s.set_leader_id(3);
  s.set_level(0);
  s.set_edge_status(1,MST); //previous one-way join (see prior test case)
  Msg m =Msg(0,1,JoinUsPayload{0,1,1,0});// 1 says to zero "Join my solo partition that I lead with level 0" across edge 0-1
  StaticQueue<Msg,32> buf;
  size_t sz;
  CHECK_EQ(OK,s.process(m,buf, sz));
  CHECK_EQ(buf.size(),0);//No action: we are 0, JOIN_US was from new_leader.
  Edge e;
  CHECK_EQ(OK,s.get_edge(1,e));
  CHECK_EQ(e.status,MST); //we marked as MST
  CHECK_EQ(s.get_leader_id(), 1);

}

TEST_CASE("unit-test join_us merge")
{
  auto s = get_state<4,32>(0,1,1,1,false,false);
  //0 is me
  //1 is unknown
  //2 is deleted
  //3 is mst & leader
  //Let's test the case where 1 sends 0 a join_us
  //and we should trigger an obsorb of 1's partition.
  s.set_leader_id(3);
  s.set_level(0);
  StaticQueue<Msg,32> buf;
  Msg m;

  //FIRST, 0 gets the message that the edge 0-1 is the MWOE from partition with leader 3.
  m =Msg(0,3,JoinUsPayload{1,0,3,0});// 3 says to zero "add the edge 1<-0 to partition 3 with level 0. 
  size_t sz;
  CHECK_EQ(OK,s.process(m,buf, sz));//shouldn't be a problem
  //ok, side effects are:
  //0 sends a message to 1 saying "Join our partition"
  CHECK_EQ(buf.size(), 1);
  Msg buf_front;
  CHECK(OK==(buf.front(buf_front)));
  CHECK_EQ(buf_front.to(), 1);
  CHECK_EQ(buf_front.from(), 0);
  CHECK_EQ(buf_front.type(), msg::Type::JOIN_US);
  CHECK_EQ(buf_front.data().join_us.join_peer, 1);//<-- edge peer
  CHECK_EQ(buf_front.data().join_us.join_root, 0);//<-- edge peer
  CHECK_EQ(buf_front.data().join_us.proposed_leader, 3);//<-- leader 
  CHECK_EQ(buf_front.data().join_us.proposed_level, 0);//<-- level is not changed during absorb
  Edge e;
  CHECK_EQ(OK, s.get_edge(1,e));
  CHECK_EQ(e.status, MST);//<-- MST link established

}


TEST_CASE("unit-test join_us merge leader-side")
{
  //essentially the other side of above test
  GhsState<4,32> s(1);
  //0 is unknown
  //1 is me
  //3 is my leader (not same as 3 above)
  s.set_leader_id(3);
  s.set_level(0);
  s.set_edge({3,1,MST,1});
  s.set_edge({0,1,UNKNOWN,1});
  s.set_parent_id(3);
  CHECK_EQ(s.get_parent_id(),3); 
  StaticQueue<Msg,32> buf;
  Msg m;

  //FIRST, 1 gets the message that the edge 1-0 is the MWOE from partition with leader 2.
  m =Msg(1,3,JoinUsPayload{0,1,3,0}); // 3 says to 1 "add the edge 1->0 to partition 3 with level 0. 
  size_t sz;
  CHECK_EQ(OK,s.process(m,buf, sz));
  //ok, side effects are:
  //1 sends a message to 0 saying "Join our partition"
  CHECK_EQ(buf.size(), 1);
  Msg buf_front;
  CHECK(OK==(buf.front(buf_front)));
  CHECK_EQ(buf_front.to(), 0);
  CHECK_EQ(buf_front.from(), 1);
  CHECK_EQ(buf_front.type(), msg::Type::JOIN_US);
  CHECK_EQ(buf_front.data().join_us.join_peer, 0);//<-- edge peer (them) 
  CHECK_EQ(buf_front.data().join_us.join_root, 1);//<-- edge root (us)
  CHECK_EQ(buf_front.data().join_us.proposed_leader, 3);//<-- leader 
  CHECK_EQ(buf_front.data().join_us.proposed_level, 0);//<-- level is not changed during absorb
  //we anticipated their response. A little. 
  Edge e;
  CHECK_EQ(s.get_edge(0,e),OK);
  CHECK_EQ(e.status, MST);
  buf.pop();

  //AND NOW, in a big twist, here comes the join message for 1-0 in the opposite direction
  CHECK_EQ(s.get_id(),1);
  CHECK_EQ(buf.size(), 0);
  m =Msg(1,3,JoinUsPayload{0,1,3,0}); 
  CHECK_EQ(OK, s.get_edge(0,e));
  CHECK_EQ(e.status, MST); //<-- still part of the gang
  CHECK_EQ(OK,s.process(m,buf, sz));
  CHECK_EQ(e.status, MST); //<-- still part of the gang, STILL
  CHECK_EQ(s.get_parent_id(),       1); //This changes because we start_round on ourselves as new leader, which process_srch's on ourselves, setting ourselves as new leader. Desired, but obtuse.
  CHECK_EQ(s.get_level(), 1); //level auto-increment b/c we detected merge()
  CHECK_EQ(s.get_leader_id(), 1); //We're leader
  //ok, side effects are:
  //1 sends a message to 0 saying "Join us"
  //AND, it sends a message to SRCH
  CHECK_EQ(buf.size(), 2); //lord only knows why it chooses this order though. 
  
  CHECK(OK==(buf.pop(buf_front)));
  CHECK_EQ(buf_front.type(), msg::Type::SRCH);
  CHECK_EQ(buf_front.to(), 3);

  Msg buf_back;
  CHECK(OK==(buf.pop(buf_back)));
  CHECK_EQ(buf_back.type(), msg::Type::SRCH); //we have a child to ping
  CHECK_EQ(buf_back.to(), 0);

}


TEST_CASE("integration-test two nodes")
{
  GhsState<4,32> s0(0); s0.add_edge(Edge{1,0,UNKNOWN,1});
  GhsState<4,32> s1(1); s1.add_edge(Edge{0,1,UNKNOWN,2});
  StaticQueue<Msg,32> buf;
  //let's turn the crank and see what happens
  size_t sz;
  s0.start_round(buf, sz);
  s1.start_round(buf, sz);
  CHECK_EQ(buf.size(),2);
  for (int i=0;i<2;i++){
    Msg m; 
    CHECK(OK==(buf.pop(m)));
    CHECK_EQ(m.type(),msg::Type::IN_PART);
    switch(m.to()){
      case (0):{ s0.process(m,buf, sz);break;}
      case (1):{ s1.process(m,buf, sz);break;}
    }
  }
  CHECK_EQ(buf.size(), 2);
  for (int i=0;i<2;i++){
    Msg m; 
    CHECK(OK==(buf.pop(m)));
    CHECK_EQ(m.type(),msg::Type::NACK_PART);
    switch(m.to()){
      case (0):{ s0.process(m,buf, sz);break;}
      case (1):{ s1.process(m,buf, sz);break;}
    }
  }
  CHECK_EQ(buf.size(), 2);
  CHECK_EQ(s0.get_level(),0); //<--NOT ++ 
  CHECK_EQ(s1.get_level(),0); //<--NOT ++ 
  for (int i=0;i<2;i++){
    Msg m; 
    CHECK(OK==(buf.pop(m)));
    CHECK_EQ(m.type(),msg::Type::JOIN_US);
    CHECK_EQ(m.from(), m.data().join_us.proposed_leader); //<-- hey can you join *my* partition with me as leader?
    switch(m.to()){
      case (0):{ s0.process(m,buf, sz);break;}
      case (1):{ s1.process(m,buf, sz);break;}
    }
  }
  CHECK_EQ(buf.size(), 1); 
  Msg buf_front;
  CHECK(OK==(buf.front(buf_front)));
  CHECK_EQ(buf_front.type(), msg::Type::SRCH); // leader triggered new round. 
  CHECK_EQ(s0.get_leader_id(),1);
  CHECK_EQ(s0.get_level(),1); //<--now ++ 
  CHECK_EQ(s1.get_level(),1); //<--now ++ 

}

TEST_CASE("integration-test opposite two nodes")
{
  GhsState<4,32> s0(0); s0.add_edge({1,0,UNKNOWN,1});
  GhsState<4,32> s1(1); s1.add_edge({0,1,UNKNOWN,2});
  StaticQueue<Msg,32> buf;
  //let's turn the crank and see what happens
  //


  //Trigger start
  size_t sz;
  s1.start_round(buf, sz);
  s0.start_round(buf, sz);

  //Nodes sent pings to neighbors
  CHECK_EQ(buf.size(),2); // IN_PART? x2
  for (int i=0;i<2;i++){
    Msg m;
    buf.pop(m);
    CHECK_EQ(m.type(),msg::Type::IN_PART);
    switch(m.to()){
      case (0):{ s0.process(m,buf, sz);break;}
      case (1):{ s1.process(m,buf, sz);break;}
    }
  }

  //Nodes recognized a partition
  CHECK_EQ(buf.size(), 2); // NACK_PART x2
  for (int i=0;i<2;i++){
    Msg m;
    buf.pop(m);
    CHECK_EQ(m.type(),msg::Type::NACK_PART);
    switch(m.to()){
      case (0):{ s0.process(m,buf, sz);break;}
      case (1):{ s1.process(m,buf, sz);break;}
    }
  }

  //Nodes responded with JOIN requests
  CHECK_EQ(buf.size(), 2); // JOIN_US x2
  CHECK_EQ(s0.get_level(),0); //<--NOT ++ 
  CHECK_EQ(s1.get_level(),0); //<--NOT ++ 
  for (int i=0;i<2;i++){
    Msg m;
    buf.pop(m);
    CHECK_EQ(m.type(),msg::Type::JOIN_US);
    CHECK_EQ(m.from(), m.data().join_us.proposed_leader); //<-- hey can you join *my* partition with me as leader?
    switch(m.to()){
      case (0):{ s0.process(m,buf, sz);break;}
      case (1):{ s1.process(m,buf, sz);break;}
    }
  }

  //Nodes updated graph:
  Edge es0, es1;
  CHECK_EQ(OK,s0.get_edge(1,es1));
  CHECK_EQ(es1.status, MST);
  CHECK_EQ(OK,s1.get_edge(0,es0));
  CHECK_EQ(es0.status, MST);

  //nodes understand leader implicitly from prior JOIN_US msgs to be max(0,1)=1
  //... 
  CHECK_EQ(s0.get_leader_id(),1);
  CHECK_EQ(s1.get_leader_id(),1);
  CHECK_EQ(s0.get_level(),1); //<--now ++ 
  CHECK_EQ(s1.get_level(),1); //<--now ++ also

  //Only one node responded, and it was the leader
  CHECK_EQ(buf.size(), 1); // SRCH from leader only
  Msg buf_front;
  CHECK(OK==(buf.front(buf_front)));
  CHECK_EQ(buf_front.type(), msg::Type::SRCH);
  CHECK_EQ(buf_front.from() , 1);

}


TEST_CASE("sim-test 3 node frenzy")
{

  std::ofstream f("sim-test-3-node-frenzy.msgs");

  GhsState<4,32> states[3]={
    {0},{1},{2}
  };
  for (int i=0;i<3;i++){
    for (int j=0;j<3;j++){
      if (i!=j){
        //add N^2 edges
        Edge to_add = {i,j,UNKNOWN,(metric_t)( (1<<i) + (1<<j))};
        states[j].set_edge(to_add);
      }
    }
  }
  StaticQueue<Msg,32> buf;
  for (int i=0;i<3;i++){
    size_t sz;
    states[i].start_round(buf, sz);
  }
  int msg_limit = 100;
  int msg_count = 0;
  //here we go:
  //
  while(buf.size()>0 && msg_count++ < msg_limit){
    Msg m; buf.pop(m);
    StaticQueue<Msg,32> added;

    f << "f: "<<states[m.from()]<<std::endl;
    f << "t: "<<states[m.to()]<<std::endl;
    f << "-  "<<m << std::endl;
    size_t sz;
    states[m.to()].process(m,added, sz);
    f << "t':"<< states[m.to()]<<std::endl;

    int added_sz = added.size();
    for (int i=0;i<added_sz;i++){
      added.pop(m);
      f <<"+  "<< m <<std::endl;
      buf.push(m);
    }
    f << std::endl;
  }

  CHECK_EQ(buf.size(),0);
  CHECK_LT(msg_count, msg_limit);

  //do we have a single partition?
  for (int i=0;i<3;i++){
    CHECK(states[i].is_converged());
    for (int j=0;j<3;j++){
      if (i!=j){
        CHECK_EQ(states[i].get_leader_id(), states[j].get_leader_id());
      }
    }
  }

}

TEST_CASE("ghs_metric")
{
  metric_t m= METRIC_NOT_SET;
  CHECK(!is_valid(m));
  m= WORST_METRIC;
  CHECK(!is_valid(m));
  m=1;
  CHECK(is_valid(m));
}
*/
