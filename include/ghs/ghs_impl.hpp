/**
 *
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
 * @file ghs_impl.hpp
 * @brief the implementation for le::ghs::GhsState
 */

#include "ghs.h"
#include "msg.h"

#ifndef NDEBUG
#include <stdexcept>
#define ghs_fatal(s) throw std::runtime_error(std::string(__FILE__)+":"+std::to_string(__LINE__)+" "+strerror(s))
#else
#define ghs_fatal(s) printf("[fatal] %s",strerror(s))
#endif

using std::max;

using namespace le::ghs::msg;

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
GhsState<MAX_AGENTS, BUF_SZ>::GhsState(agent_t my_id) {
  this->my_id =  my_id;
  reset();
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
GhsState<MAX_AGENTS, BUF_SZ>::~GhsState(){}

/**
 * Reset the algorithm status completely
 */
template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::reset() {
  set_leader_id(my_id);
  set_level(LEVEL_START);
  set_parent_id(my_id);

  n_peers=0;
  for (size_t i=0;i<MAX_AGENTS;i++){
    //peers[i] = ??
    outgoing_edges[i].status=UNKNOWN;
    waiting_for_response[i]=false;
    response_required[i]=false;
    response_prompt[i]={};
  }
  this->best_edge            =  worst_edge();
  this->algorithm_converged  =  false;

  return OK;
}


/**
 * Queue up the start of the round
 *
 */
template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::start_round(StaticQueue<Msg,BUF_SZ> &outgoing_buffer, size_t & qsz) {
  //If I'm leader, then I need to start the process. Otherwise wait.
  if (my_leader == my_id){
    //nobody tells us what to do but ourselves
    return process_srch(my_id, {my_leader, my_level}, outgoing_buffer, qsz);
  }
  qsz=0;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
Edge GhsState<MAX_AGENTS, BUF_SZ>::mwoe() const {
  return best_edge;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::process(const Msg &msg, StaticQueue<Msg,BUF_SZ> &outgoing_buffer, size_t &qsz) {

  if (msg.from()==my_id){
    //ghs_debug_crash("Received msg.from() self!");
    return PROCESS_SELFMSG;
  }

  if (msg.to() != my_id){
    return PROCESS_NOTME;
  }

  if (! has_edge(msg.from()) ){
    return PROCESS_NO_EDGE_FOUND;
  }

  switch (msg.type()){
    case    (msg::Type::SRCH):{         return  process_srch(         msg.from(), msg.data().srch, outgoing_buffer, qsz);  }
    case    (msg::Type::SRCH_RET):{     return  process_srch_ret(     msg.from(), msg.data().srch_ret, outgoing_buffer, qsz);  }
    case    (msg::Type::IN_PART):{      return  process_in_part(      msg.from(), msg.data().in_part, outgoing_buffer, qsz);  }
    case    (msg::Type::ACK_PART):{     return  process_ack_part(     msg.from(), msg.data().ack_part, outgoing_buffer, qsz);  }
    case    (msg::Type::NACK_PART):{    return  process_nack_part(    msg.from(), msg.data().nack_part, outgoing_buffer, qsz);  }
    case    (msg::Type::JOIN_US):{      return  process_join_us(      msg.from(), msg.data().join_us, outgoing_buffer, qsz);  }
    case    (msg::Type::NOOP):{         return  process_noop(         outgoing_buffer , qsz); }
    default:{ return PROCESS_INVALID_TYPE; }
  }
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::process_srch(  agent_t from, const msg::SrchPayload& data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{

  //this msg is weird, in that we sometimes trigger internally with from==my_id

  if (from!=my_id){//must be MST edge then
    Edge to_them;
    auto ret = get_edge(from, to_them);
    if (ret != OK) 
    { 
      return ret; 
    }
    if (to_them.status!=MST ) 
    { 
      return PROCESS_REQ_MST; 
    }
  }

  if (waiting_count() != 0 )
  {
    return SRCH_STILL_WAITING;
  }

  //grab the new partition information, since only one node / partition sends srch() msgs.
  agent_t leader = data.your_leader;
  level_t   level  = data.your_level;
  my_leader = leader;
  my_level  = level;
  //also note our parent may have changed
  parent=from;

  //initialize the best edge to a bad value for comparisons
  best_edge = worst_edge();
  best_edge.root = my_id;

  //we'll cache outgoing messages temporarily
  StaticQueue<Msg,BUF_SZ> srchbuf;

  //first broadcast the SRCH down the tree
  msg::Data to_send;
  to_send.srch = SrchPayload{my_leader, my_level};
  size_t srch_sent=0;
  le::Errno srch_ret = mst_broadcast(msg::Type::SRCH, to_send, srchbuf,srch_sent);
  if (srch_ret!=OK){
    return srch_ret;
  }

  //then ping unknown edges
  //OPTIMIZATION: Ping neighbors in sorted order, rather than flooding

  to_send.in_part = InPartPayload{my_leader, my_level};
  size_t part_sent=0;
  le::Errno part_ret = typecast(status_t::UNKNOWN, msg::Type::IN_PART, to_send, srchbuf, part_sent);
  if (part_ret!=OK){
    return part_ret;
  }

  //remember who we sent to so we can wait for them:
  size_t srchbuf_sz = srchbuf.size();
  if (srchbuf_sz != srch_sent + part_sent){
    return ERR_QUEUE_MSGS;
  }

  //at this point, we may not have sent any msgs, because:
  //1) There are no unknown outgoing edges
  //2) There are no children to relay the srch to
  //
  //If that's the case, we can safely respond with "No MWOE" and that's it.
  if (srchbuf_sz == 0 && delayed_count() ==0){
    return respond_no_mwoe(buf,qsz);
  }

  //past here, we know we either had srchbuf msgs, or a delayed msg. 

  //first handle srchbuf msgs
  //push temporarily cache'd messages to outgoingn buf, and note the receiver
  //ID so we can track their response later.
  size_t buf_sz_before = buf.size();
  for (size_t i=0;i<srchbuf_sz;i++){
    Msg m;
    srchbuf.pop(m);
    set_waiting_for(m.to(), true);
    buf.push(m);
  }

  if ( (buf.size() - buf_sz_before != srch_sent + part_sent )  )
  {
    return ERR_QUEUE_MSGS;
  }

  //make sure to check_new_level, since our level may have changed, above,
  //which will handled delayed_count != 0;
  size_t old_msgs_processed=0;
  le::Errno lvl_err = check_new_level(buf,old_msgs_processed);
  if (lvl_err != OK ){
    return lvl_err;
  }

  //notify hunky dory
  qsz = srch_sent + part_sent + old_msgs_processed;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::respond_no_mwoe( StaticQueue<Msg, BUF_SZ> &buf, size_t & qsz)
{
  msg::Data pld;
  pld.srch_ret.to=0;
  pld.srch_ret.from=0;
  pld.srch_ret.metric = WORST_METRIC;
  return mst_convergecast(msg::Type::SRCH_RET, pld, buf,qsz);
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::process_srch_ret(  agent_t from, const SrchRetPayload &data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{


  if (this->waiting_count() == 0){
    return UNEXPECTED_SRCH_RET;
  }

  bool wf=false;
  auto wfr = is_waiting_for(from,wf);
  if (OK!=wfr){
    return wfr;
  }
  if ( !wf ){
    return UNEXPECTED_SRCH_RET;
  }

  auto swfr = set_waiting_for(from,false);
  if (OK != swfr ){
    return swfr;
  }

  //compare our best edge to their best edge
  //first, get their best edge
  Edge theirs;
  theirs.peer        =  data.to;
  theirs.root        =  data.from;
  theirs.metric_val  =  data.metric;

  if (theirs.metric_val < best_edge.metric_val){
    best_edge.root        =  theirs.root;
    best_edge.peer        =  theirs.peer;
    best_edge.metric_val  =  theirs.metric_val;
  }

  return check_search_status(buf,qsz);
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::process_in_part(  agent_t from, const InPartPayload& data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{
  //let them know if we're in their partition or not. Easy.
  agent_t part_id = data.leader;

  //except if they are *ahead* of us in the execution of their algorithm. That is, what if we
  //don't actually know if we are in their partition or not? This is detectable if their level > ours. 
  level_t their_level   = data.level;
  level_t our_level     = my_level;

  if (their_level <= our_level){
    //They aren't behind, so we can respond
    if (part_id == this->my_leader){
      Msg to_send (from, my_id, AckPartPayload{});
      buf.push( to_send );
      //do not do this: 
      //waiting_for.erase(from);
      //set_edge_status(from,DELETED);
      //(breaks the contract of IN_PART messages, because now we don't need a
      //response to the one we must have sent to them.
      qsz=1;
      return OK;
    } else {
      Msg to_send (from, my_id, NackPartPayload{});
      buf.push (to_send); 
      qsz=1;
      return OK;
    } 
  } else {
    respond_later(from, data);
    qsz=0;
    return OK;
  }
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::process_ack_part(  agent_t from, const AckPartPayload& data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{
  //is this the right time to receive this msg?
  bool wf=false; 
  auto wfr = is_waiting_for(from,wf);
  if (OK!=wfr){
    return wfr;
  }
  if (!wf)
  {
    return ACK_NOT_WAITING;
  }

  //we now know that the sender is in our partition. Mark their edge as deleted
  auto sesr = set_edge_status(from, DELETED);
  if (OK != sesr){
    return sesr;
  }

  auto swfr = set_waiting_for(from, false);
  if (OK != swfr){
    return swfr;
  }

  return check_search_status(buf,qsz);
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::process_nack_part(  agent_t from, const NackPartPayload &data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{
  //is this the right time to receive this msg?
  bool wf=false; 
  auto wfr = is_waiting_for(from,wf);
  if (OK!=wfr){
    return wfr;
  }
  if (!wf)
  {
    return ACK_NOT_WAITING;
  }

  Edge their_edge;
  //although has_edge was checked, we are careful anyway
  auto ger = get_edge(from, their_edge);
  if (OK != ger)
  {
    return PROCESS_NO_EDGE_FOUND;
  }
  
  if (best_edge.metric_val > their_edge.metric_val){
    best_edge = their_edge;
  }

  auto swfr = set_waiting_for(from,false);
  if (OK != swfr){
    return swfr;
  }

  return check_search_status(buf,qsz);
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::check_search_status(StaticQueue<Msg,BUF_SZ> &buf, size_t & qsz){
  
  if (waiting_count() == 0)
  {
    auto e              = mwoe();
    bool am_leader      = (my_leader == my_id);
    bool found_new_edge = (e.metric_val < WORST_METRIC);
    bool its_my_edge    = (mwoe().root == my_id);

    if (!am_leader){
      //pass on results, no matter how bad
      msg::Data send_data;
      send_data.srch_ret = SrchRetPayload{e.peer, e.root, e.metric_val};
      return mst_convergecast( msg::Type::SRCH_RET, send_data, buf, qsz);
    }

    if (am_leader && found_new_edge && its_my_edge){
      if (e.peer == e.root){
        return BAD_MSG;
      }
      //just start the process to join up, rather than sending messages
      return process_join_us(my_id, {e.peer, e.root, get_leader_id(), get_level()}, buf, qsz);
    }

    if (am_leader && !found_new_edge ){
      //I'm leader, no new edge, let's move on b/c we're done here
      return process_noop( buf, qsz);
    }

    if (am_leader && found_new_edge && !its_my_edge){
      //inform the crew to add the edge
      //This is a bit awkward ... 
      msg::Data data;
      data.join_us = JoinUsPayload{e.peer, e.root, get_leader_id(), get_level()};
      return mst_broadcast( JOIN_US, data, buf, qsz);
    }
  } 

  //nothing to do
  qsz=0;
  return OK;
}


template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::check_new_level( StaticQueue<Msg,BUF_SZ> &buf, size_t & qsz){

  qsz=0;
  for (size_t idx=0;idx<n_peers;idx++){
    if (response_required[idx]){
      agent_t who = peers[idx];
      InPartPayload &m = response_prompt[idx];
      level_t their_level = m.level; 
      if (their_level <= get_level() )
      {
        size_t sentsz=0;
        //ok to answer, they were waiting for us to catch up
        le::Errno ret=process_in_part(who, m, buf, sentsz);
        if (ret!=OK){
          //some error, propegate up
          return ret;
        }
        qsz+=sentsz;
        response_required[idx]=false;
      } 
    }
  }
  return OK;
}


template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::process_join_us(  agent_t from, const JoinUsPayload &data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{

  auto join_peer  = data.join_peer; // the side of the edge that is in the other partition 
  auto join_root  = data.join_root; // the side of the edge that is in the partition initiating join
  auto join_lead  = data.proposed_leader; // the leader, as declared during search, of the peer
  auto join_level = data.proposed_level; // the level of the peer as declared during search

  bool not_involved            = (join_root != my_id && join_peer != my_id);
  bool in_initiating_partition = (join_root == my_id);

  if (not_involved){
    if (join_lead != my_leader){
      return JOIN_BAD_LEADER;
    }
    if (join_level != my_level){
      return JOIN_BAD_LEVEL;
    }  

    msg::Data to_send;
    to_send.join_us = data;
    return mst_broadcast(msg::Type::JOIN_US, to_send, buf, qsz);
  }

  //let's find the edge to the other partition
  Edge edge_to_other_part;

  if (in_initiating_partition){
    //leader CAN be different, even though we're initating, IF we're on an MST
    //link to them
    le::Errno retcode;
    Edge join_peer_edge;
    retcode = get_edge(join_peer, join_peer_edge);
    if (retcode != OK){
      return retcode;
    }

    //check join_peer status (better be MST)
    if (join_lead != my_leader && join_peer_edge.status != MST){
      return JOIN_INIT_BAD_LEADER;
    }
    if (join_level != my_level){
      return JOIN_INIT_BAD_LEVEL;
    }  
    //found the correct edge
    retcode = get_edge(join_peer, edge_to_other_part);
    if (OK != retcode)
    { 
      return retcode; 
    }
  } else {
    //we aren't in initiating partition, and yet it includes our partition, this is a problem!
    if (join_lead == my_leader){
      return JOIN_MY_LEADER;
    }
    //level can be same, lower (from another partition), but not higher (we shouldn't have replied)
    if (join_level > my_level){
      return JOIN_UNEXPECTED_REPLY;
    }  
    //found the correct edge
    auto ger = get_edge(join_root, edge_to_other_part);
    if (OK != ger)
    { 
      return ger; 
    }
  }

  //after all that, we found the edge

  if ( edge_to_other_part.status == MST){
    //we already absorbed once, so now we merge()
    auto leader_id = max(join_peer, join_root);
    my_leader = leader_id;
    my_level++;
    if (leader_id == my_id){
      //In this case, we already sent JOIN_US (since it's an MST link), and if
      //they have not procssed that, they will soon enough. At that time, they
      //will see the link to us as MST (after all they sent JOIN_US -- this
      //msg), AND they will recognize our leader-hood. We advance in faith that
      //they will march along. 
      return start_round(buf, qsz);
    } else {
      //In this case, we already sent JOIN_US (b/c it's an MST link), meaning
      //this came from THEM. If they rec'd ours first, then they know if they
      //are leader or not. if not, AND we're not leader, we're at deadlock
      //until they process our JOIN_US and see the MST link from their JOIN_US
      //request and recognize their own leader-hood. We wait. 
      qsz=0;
      return OK;
    } 
  } else if (edge_to_other_part.status == UNKNOWN) {
      if (in_initiating_partition ){
        //In this case, we're sending a JOIN_US without hearing from them yet.
        //We may not be their MWOE, which would make this an "absorb" case.
        //just send it, see what they say (see next one). btw, because we were
        //able to find a MWOE, we know that their level >= ours. Otherwise,
        //they would not have responded to our search (see process_in_part). So
        //this absorb request is valid and setting their link as MST is OK. 
        set_edge_status(join_peer, MST);
        auto payload = JoinUsPayload{ data.join_peer, data.join_root, 
            data.proposed_leader, data.proposed_level };
        Msg to_send = Msg( join_peer, my_id, payload ); 
        buf.push(to_send);
        qsz=1;
        return OK;
      } else {

        //In this case, we received a JOIN_US from another partition, one that
        //we have not yet recognized or marked as our own MWOE. This means they
        //are a prime candidate to absorb into our partition. 
        //NOTE, if we were waiting for them, they would not respond until their
        //level is == ours, so this should never fail:
        if (my_level < join_level){
          return JOIN_UNEXPECTED_REPLY;
        }

        //Since we know they are prime absorbtion material, we just do it and
        //mark them as children. There's one subtlety here: We may have to
        //revise our search status once they absorb!! That's TODO: test to see
        //if we adequately handle premature absorb requests. After all, our
        //MWOE might now be in their subtree. 
        auto sesr = set_edge_status(join_root, MST);
        if (OK != sesr){
          return sesr;
        }
        
        //Anyway, because we aren't in the initiating partition, the other guy
        //already has us marked MST, so we don't need to do anything.  -- a
        //leader somewhere else will send the next round start (or perhaps it
        //will be one of us when we do a merge()
        qsz=0;
        return OK;
      }   
    } else {
      ghs_fatal(ERR_IMPL);
      return ERR_IMPL;
    }

  ghs_fatal(ERR_IMPL);
  return ERR_IMPL;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::process_noop(StaticQueue<Msg,BUF_SZ> &buf, size_t &qsz){
  algorithm_converged=true;
  return mst_broadcast(msg::Type::NOOP, {},buf, qsz);
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::typecast(const status_t status, const msg::Type m, const msg::Data &data, StaticQueue<Msg,BUF_SZ> &buf, size_t &qsz)const {
  size_t sent=0;
  for (size_t idx=0;idx<n_peers;idx++){
    const Edge &e = outgoing_edges[idx];
    if (e.root!=my_id){
      return CAST_INVALID_EDGE;
    }
    if ( e.status == status ){
      sent++;
      Msg to_send (e.peer, my_id, m, data);
      buf.push( to_send );
    }
  }
  qsz=sent;
  return OK;
}


template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::mst_broadcast(const msg::Type m, const msg::Data &data, StaticQueue<Msg,BUF_SZ> &buf, size_t&qsz)const {
  size_t sent =0;
  for (size_t idx =0;idx<n_peers;idx++){
    const Edge&e = outgoing_edges[idx];
    if (e.root!=my_id){
      return CAST_INVALID_EDGE;
    }
    if (e.status == MST && e.peer != parent){
      sent++;
      Msg to_send( e.peer, my_id, m, data);
      buf.push( to_send );
    }
  }
  qsz=sent;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::mst_convergecast(const msg::Type m, const msg::Data& data, StaticQueue<Msg,BUF_SZ> &buf, size_t &qsz)const {
  size_t sent=0;
  for (size_t idx =0;idx<n_peers;idx++){
    const Edge&e = outgoing_edges[idx];
    if (e.root!=my_id){
      return CAST_INVALID_EDGE;
    }
    if (e.status == MST && e.peer == parent){
      sent++;
      Msg to_send ( e.peer, my_id, m, data);
      buf.push( to_send );
    }
  }
  qsz=sent;
  return OK;
}


template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::set_parent_id(const agent_t& id) {

  //case 1: self-loop ok
  if (id==get_id()){
    parent = id; 
    return OK;
  }

  //case 2: MST links ok
  if (!has_edge(id)){
    return PARENT_UNRECOGNIZED;
  }
  Edge peer;
  auto ger = get_edge(id,peer);
  if (OK != ger){
    return ger;
  }
  if (peer.status == MST ){
    parent = id; 
    return OK;
  }
  return PARENT_REQ_MST;
}


template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
agent_t GhsState<MAX_AGENTS, BUF_SZ>::get_parent_id() const {
  return parent;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
agent_t GhsState<MAX_AGENTS, BUF_SZ>::get_leader_id() const {
  return my_leader;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::set_leader_id(const agent_t& leader) {
  my_leader = leader;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
level_t GhsState<MAX_AGENTS, BUF_SZ>::get_level() const {
  return my_level;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::set_level(const level_t & l){
  my_level = l;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
bool GhsState<MAX_AGENTS, BUF_SZ>::is_converged() const {
  return algorithm_converged;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::checked_index_of(const agent_t& who, size_t &idx) const{
  if (who == my_id){
    return IMPL_REQ_PEER_MY_ID;
  }
  for (size_t i=0;i<n_peers;i++){
    if (peers[i] == who){
      idx = i;
      return OK;
    }
  }
  return NO_SUCH_PEER;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::set_waiting_for(const agent_t &who, bool waiting){

  size_t idx;
  le::Errno retcode=checked_index_of(who,idx);
  if (retcode!=OK){return retcode;}

  waiting_for_response[idx]=waiting;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::is_waiting_for(const agent_t& who, bool& waiting_for){
  size_t idx;
  le::Errno retcode=checked_index_of(who,idx);
  if (retcode!=OK){return retcode;}

  waiting_for = waiting_for_response[idx];
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::set_response_required(const agent_t &who, bool resp)
{
  size_t idx;
  le::Errno retcode=checked_index_of(who,idx);
  if (retcode!=OK){return retcode;}

  response_required[idx]=resp;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::is_response_required(const agent_t &who, bool &res_req ){
  size_t idx;
  le::Errno retcode=checked_index_of(who,idx);
  if (retcode!=OK){return retcode;}

  res_req = response_required[idx];
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::set_response_prompt(const agent_t &who, const InPartPayload& m){
  size_t idx;
  le::Errno retcode=checked_index_of(who,idx);
  if (retcode!=OK){return retcode;}

  response_prompt[idx]=m;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>:: get_response_prompt(const agent_t &who, InPartPayload &out){
  size_t idx;
  le::Errno retcode=checked_index_of(who,idx);
  if (retcode!=OK){return retcode;}

  out = response_prompt[idx];
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::respond_later(const agent_t&from, const InPartPayload m)
{
  size_t idx;
  le::Errno retcode=checked_index_of(from,idx);
  if (retcode!=OK){return retcode;}

  response_required[idx]=true;
  response_prompt[idx]  =m;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
bool GhsState<MAX_AGENTS, BUF_SZ>::has_edge(const agent_t to) const{
  size_t idx;
  return OK==checked_index_of(to,idx);
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::get_edge(const agent_t& to, Edge &out)  const
{
  size_t idx;
  le::Errno retcode=checked_index_of(to,idx);
  if (retcode!=OK){return retcode;}

  out = outgoing_edges[idx];
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::get_edge_status(const agent_t& to, status_t & out)  const
{
  size_t idx;
  le::Errno retcode=checked_index_of(to,idx);
  if (retcode!=OK){return retcode;}

  out = outgoing_edges[idx].status;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>


le::Errno GhsState<MAX_AGENTS, BUF_SZ>::set_edge_status(const agent_t &to, const status_t &status)
{
  size_t idx;
  le::Errno retcode=checked_index_of(to,idx);
  if (retcode!=OK){return retcode;}

  outgoing_edges[idx].status=status;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::get_edge_metric(const agent_t& to, metric_t & out)  const
{
  size_t idx;
  le::Errno retcode=checked_index_of(to,idx);
  if (retcode!=OK){return retcode;}

  out = outgoing_edges[idx].metric_val;
  return OK;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::set_edge_metric(const agent_t &to, const metric_t m)
{
  size_t idx;
  le::Errno retcode=checked_index_of(to,idx);
  if (retcode!=OK){return retcode;}

  outgoing_edges[idx].metric_val=m;
  return OK;
}


template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>

le::Errno GhsState<MAX_AGENTS, BUF_SZ>::add_edge_to(const agent_t& who ) {
  Edge e;
  e.peer=who;
  e.root=my_id;
  e.status=UNKNOWN;
  e.metric_val=0;
  return add_edge(e);
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
le::Errno GhsState<MAX_AGENTS, BUF_SZ>::set_edge(const Edge &e) {

  if (e.root != my_id){
    return SET_INVALID_EDGE;
  }

  agent_t who = e.peer;
  size_t idx;
  le::Errno er = checked_index_of(who,idx);
  if (OK == er)
  {
    //found em
    outgoing_edges[idx].metric_val  =  e.metric_val;
    outgoing_edges[idx].status      =  e.status;
    return OK;
  } 
  else if (NO_SUCH_PEER == er)
  {
    //don't have em (yet)
    if (n_peers>=MAX_AGENTS){
      return TOO_MANY_AGENTS;
    }
    peers[n_peers]=e.peer;
    outgoing_edges[n_peers] = e;
    n_peers++;
    return OK;
  } 
  else 
  {
    //something else happened
    return er;
  }
  ghs_fatal(ERR_IMPL);
  return ERR_IMPL;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
size_t GhsState<MAX_AGENTS, BUF_SZ>::waiting_count() const 
{
  size_t waiting =0;
  //count them up. 
  for (size_t i=0;i<n_peers;i++){
    if (waiting_for_response[i]){
      waiting++;
    }
  }
  return waiting;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
size_t GhsState<MAX_AGENTS, BUF_SZ>::delayed_count() const 
{
  size_t delayed =0;
  for (size_t i=0;i<n_peers;i++){
    if (response_required[i]){
      delayed++;
    }
  }
  return delayed;
}

template <std::size_t MAX_AGENTS, std::size_t BUF_SZ>
agent_t GhsState<MAX_AGENTS, BUF_SZ>::get_id() const {
  return my_id;
}

