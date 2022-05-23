#include "ghs.hpp"
#include "msg.hpp"
#include <algorithm>
#include <assert.h>
#include <deque>
#include <stdexcept>
#include <sstream>
#include <string>

using std::max;

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsState<GHS_MAX_AGENTS, BUF_SZ>::GhsState(AgentID my_id) noexcept{
  this->my_id =  my_id;
  reset();
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsState<GHS_MAX_AGENTS, BUF_SZ>::~GhsState(){}

/**
 * Reset the algorithm status completely
 */
template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::reset() noexcept{
  set_leader_id(my_id);
  set_level(GHS_LEVEL_START);
  set_parent_id(my_id);

  n_peers=0;
  for (size_t i=0;i<GHS_MAX_AGENTS;i++){
    //peers[i] = ??
    outgoing_edges[i].status=UNKNOWN;
    waiting_for_response[i]=false;
    response_required[i]=false;
    response_prompt[i]={};
  }
  this->best_edge            =  ghs_worst_possible_edge();
  this->algorithm_converged  =  false;

  return GHS_OK;
}


/**
 * Queue up the start of the round
 *
 */
template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::start_round(StaticQueue<Msg,BUF_SZ> &outgoing_buffer, size_t & qsz) noexcept{
  //If I'm leader, then I need to start the process. Otherwise wait.
  if (my_leader == my_id){
    //nobody tells us what to do but ourselves
    return process_srch(my_id, {my_leader, my_level}, outgoing_buffer, qsz);
  }
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
Edge GhsState<GHS_MAX_AGENTS, BUF_SZ>::mwoe() const noexcept{
  return best_edge;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process(const Msg &msg, StaticQueue<Msg,BUF_SZ> &outgoing_buffer, size_t &qsz) noexcept{
  if (msg.from == my_id ){
    assert(false && "Received message from self!");
    return GHS_MSG_INVALID_SENDER;
  }
  if (msg.to != my_id){
    assert(false && "Received message to someone else!");
    return GHS_MSG_INVALID_SENDER;
  }
  switch (msg.type){
    case    (MsgType::SRCH):{         return  process_srch(         msg.from, msg.data.srch, outgoing_buffer, qsz);  }
    case    (MsgType::SRCH_RET):{     return  process_srch_ret(     msg.from, msg.data.srch_ret, outgoing_buffer, qsz);  }
    case    (MsgType::IN_PART):{      return  process_in_part(      msg.from, msg.data.in_part, outgoing_buffer, qsz);  }
    case    (MsgType::ACK_PART):{     return  process_ack_part(     msg.from, msg.data.ack_part, outgoing_buffer, qsz);  }
    case    (MsgType::NACK_PART):{    return  process_nack_part(    msg.from, msg.data.nack_part, outgoing_buffer, qsz);  }
    case    (MsgType::JOIN_US):{      return  process_join_us(      msg.from, msg.data.join_us, outgoing_buffer, qsz);  }
    case    (MsgType::NOOP):{         return  process_noop( outgoing_buffer ); }
    default:{ 
              assert(false&&"GHS Received invalid message type");
              return GHS_MSG_INVALID_TYPE; 
            }
  }
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_srch(  AgentID from, const SrchPayload& data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{
  //we either sent to ourselves, OR we should have an edge to them
  if (!has_edge(from) && from!=my_id){
    assert(false&&"GHS Received msg from someone we do no have an edge to (and wasn't us!)");
    return GHS_MSG_INVALID_SENDER;
  }
  
  if (from !=my_id && get_edge(from).status !=MST){
    assert(false&&"GHS Received SRCH msg from someone off the MST!");
    return GHS_MSG_INVALID_SENDER;
  }

  assert(from == my_id ||  get_edge(from).status == MST); // now that would be weird if it wasn't

  if (this->waiting_count() != 0 )
  {
    assert(false&&" Waiting but got SRCH ");
    return GHS_INVALID_STATE;
  }

  assert(this->waiting_count() ==0 && " We got a srch msg while still waiting for results!");

  //grab the new partition information, since only one node / partition sends srch() msgs.
  auto leader = data.your_leader;
  auto level  = data.your_level;
  my_leader = leader;
  my_level = level;
  //also note our parent may have changed
  parent=from;

  //initialize the best edge to a bad value for comparisons
  best_edge = ghs_worst_possible_edge();
  best_edge.root = my_id;

  //we'll cache outgoing messages temporarily
  StaticQueue<Msg,BUF_SZ> srchbuf;

  //first broadcast
  MsgData to_send;
  to_send.srch = SrchPayload{my_leader, my_level};
  size_t srch_sent=0;
  GhsError srch_ret = mst_broadcast(MsgType::SRCH, to_send, srchbuf,srch_sent);
  GhsAssert(srch_ret);

  //then ping unknown edges
  //OPTIMIZATION: Ping neighbors in sorted order, rather than flooding

  to_send.in_part = InPartPayload{my_leader, my_level};
  size_t part_sent=0;
  GhsError part_ret = typecast(EdgeStatus::UNKNOWN, MsgType::IN_PART, to_send, srchbuf, part_sent);
  GhsAsserT(part_ret);

  //remember who we sent to so we can wait for them:
  size_t srchbuf_sz = srchbuf.size();
  assert(srchbuf_sz == srch_sent + part_sent);

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
    set_waiting_for(m.to, true);
    buf.push(m);
  }

  //asserts are factored out in release code, so tell compiler to ignore that
  //variable.
  if ( (buf.size() - buf_sz_before != srch_sent + part_sent )  )
  {
    assert(false&&"Our buffer had too many or two few messages to send after we processed");
  }

  //make sure to check_new_level, since our level may have changed, above,
  //which will handled delayed_count != 0;
  size_t old_msgs_processed=0;
  GhsError new_lvl = check_new_level(buf,old_msgs_processed);
  GhsAssert(new_lvl);

  qsz = srch_sent + part_sent + old_msgs_processed;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::respond_no_mwoe( StaticQueue<Msg, BUF_SZ> &buf, size_t & qsz)
{
  MsgData pld;
  pld.srch_ret.to=0;
  pld.srch_ret.from=0;
  pld.srch_ret.metric = ghs_worst_possible_edge().metric_val;
  return mst_convergecast(SRCH_RET, pld, buf,qsz);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_srch_ret(  AgentID from, const SrchRetPayload &data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{

  if (this->waiting_count() == 0){
    assert(false && "Got a SRCH_RET but we aren't waiting for anyone -- did we reset()?");
    return GHS_INVALID_SRCHR_REC;
  }

  if (! is_waiting_for(from) )
  {
    assert(false && "Got a SRCH_RET from node we aren't waiting_for");
    return GHS_INVALID_SRCHR_REC;
  }

  set_waiting_for(from,false);

  //compare our best edge to their best edge
  //
  //first, get their best edge
  //
  //assert(data.size()==3);
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

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_in_part(  AgentID from, const InPartPayload& data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{
  //let them know if we're in their partition or not. Easy.
  AgentID part_id = data.leader;

  //except if they are *ahead* of us in the execution of their algorithm. That is, what if we
  //don't actually know if we are in their partition or not? This is detectable if their level > ours. 
  Level their_level   = data.level;
  Level our_level     = my_level;

  assert(has_edge(from));

  if (their_level <= our_level){
    //They aren't behind, so we can respond
    if (part_id == this->my_leader){
      Msg to_send = AckPartPayload{}.to_msg(from,my_id);
      buf.push( to_send );
      //do not do this: 
      //waiting_for.erase(from);
      //set_edge_status(from,DELETED);
      //(breaks the contract of IN_PART messages, because now we don't need a
      //response to the one we must have sent to them.
      qsz=1;
      return GHS_OK;
    } else {
      Msg to_send = NackPartPayload{}.to_msg(from, my_id);
      buf.push (to_send); 
      qsz=1;
      return GHS_OK;
    } 
  } else {
    respond_later(from, data);
    qsz=0;
    return GHS_OK;
  }
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_ack_part(  AgentID from, const AckPartPayload& data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{
  //we now know that the sender is in our partition. Mark their edge as deleted
  if (!is_waiting_for(from))
  {
    assert(false && "We got a IN_PART message from an agent, but we weren't waiting for one from them");
    return GHS_INVALID_INPART_REC;
  }
  set_edge_status(from, DELETED);
  set_waiting_for(from, false);
  return check_search_status(buf,qsz);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_nack_part(  AgentID from, const NackPartPayload &data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{
  //we now know that the sender is in our partition. Mark their edge as deleted
  if (!is_waiting_for(from)){
    assert(false && "We got a IN_PART message from an agent, but we weren't waiting for one from them");
    return GHS_INVALID_STATE;
  }
  assert(has_edge(from));
  Edge their_edge = get_edge( from );
  
  if (best_edge.metric_val > their_edge.metric_val){
    best_edge = their_edge;
  }

  //asserts:
  set_waiting_for(from,false);
  return check_search_status(buf,qsz);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::check_search_status(StaticQueue<Msg,BUF_SZ> &buf, size_t & qsz){
  
  if (waiting_count() == 0)
  {
    auto e              = mwoe();
    bool am_leader      = (my_leader == my_id);
    bool found_new_edge = (e.metric_val < ghs_worst_possible_edge().metric_val);
    bool its_my_edge    = (mwoe().root == my_id);

    if (!am_leader){
      //pass on results, no matter how bad
      MsgData send_data;
      send_data.srch_ret = SrchRetPayload{e.peer, e.root, e.metric_val};
      return mst_convergecast( MsgType::SRCH_RET, send_data, buf, qsz);
    }

    if (am_leader && found_new_edge && its_my_edge){
      //just start the process to join up, rather than sending messages
      assert(e.peer != e.root); //ask me why I check

      return process_join_us(my_id, {e.peer, e.root, get_leader_id(), get_level()}, buf, qsz);
    }

    if (am_leader && !found_new_edge ){
      //I'm leader, no new edge, let's move on b/c we're done here
      return process_noop( buf, qsz);
    }

    if (am_leader && found_new_edge && !its_my_edge){
      //inform the crew to add the edge
      auto msg = JoinUsPayload{e.peer, e.root, get_leader_id(), get_level()}.to_msg(0,0);
      return mst_broadcast( msg.type, msg.data, buf, qsz);
    }
  } 

  return 0;
}


template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::check_new_level( StaticQueue<Msg,BUF_SZ> &buf, size_t & qsz){

  for (size_t idx=0;idx<n_peers;idx++){
    if (response_required[idx]){
      AgentID who = peers[idx];
      InPartPayload &m = response_prompt[idx];
      Level their_level = m.level; 
      if (their_level <= get_level() )
      {
        //ok to answer, they were waiting for us to catch up
        GhsError ret=process_in_part(who, m, buf, qsz);
        GhsAssert(ret);
        response_required[idx]=false;
      } 
    }
  }

  return GHS_OK;
}


template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_join_us(  AgentID from, const JoinUsPayload &data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{

  //we preserve the opportunity to trigger our own joins here with from==my_id
  assert( has_edge(from) || from==my_id);

  auto join_peer  = data.join_peer; // the side of the edge that is in the other partition 
  auto join_root  = data.join_root; // the side of the edge that is in the partition initiating join
  auto join_lead  = data.proposed_leader; // the leader, as declared during search, of the peer
  auto join_level = data.proposed_level; // the level of the peer as declared during search

  bool not_involved            = (join_root != my_id && join_peer != my_id);
  bool in_initiating_partition = (join_root == my_id);

  if (not_involved){
    if (join_lead != my_leader){
      assert(false && "We should never receive a JOIN_US with a different leader, unless we are on the partition boundary");
      return GHS_INVALID_JOIN_REC;
    }
    if (join_level != my_level){
      assert(false && "We should never receive a JOIN_US with a different level, unless we are on the partition boundary");
      return GHS_INVALID_JOIN_REC;
    }  

    MsgData to_send;
    to_send.join_us = data;
    return mst_broadcast(MsgType::JOIN_US, to_send, buf, qsz);
  }

  Edge edge_to_other_part;

  if (in_initiating_partition){
    //leader CAN be different, even though we're initating, IF we're on an MST
    //link to them
    assert(has_edge(join_peer));
    Edge join_peer_edge = get_edge(join_peer);

    if (join_lead != my_leader && join_peer_edge.status != MST){
      assert(false && "We should never receive a JOIN_US with a different leader when we initiate");
      return GHS_INVALID_JOIN_REC;
    }
    if (join_level != my_level){
      assert(false && "We should never receive a JOIN_US with a different level when we initiate");
      return GHS_INVALID_JOIN_REC;
    }  
    assert(has_edge(join_peer));
    edge_to_other_part  = get_edge(join_peer);
  } else {
    if (join_lead == my_leader){
      assert(false && "We should never receive a JOIN_US with same leader from a different partition");
      return GHS_INVALID_JOIN_REC;
    }
    //level can be same, lower (from another partition), but not higher (we shouldn't have replied)
    if (join_level > my_level){
      assert(false && "We should never receive a JOIN_US with a higher level when we do not initiate (we shouldnt have replied to their IN_PART)");
      return GHS_INAVALID_JOIN_REC;
    }  
    assert(has_edge(join_root));
    edge_to_other_part = get_edge(join_root);
  }

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
      return GHS_OK;
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
        Msg to_send = data.to_msg(join_peer,my_id);
        buf.push(to_send);
        qsz=1;
        return GHS_OK;
      } else {

        assert(!in_initiating_partition);

        //In this case, we received a JOIN_US from another partition, one that
        //we have not yet recognized or marked as our own MWOE. This means they
        //are a prime candidate to absorb into our partition. 
        //NOTE, if we were waiting for them, they would not respond until their
        //level is == ours, so this should never fail:
        assert(my_level>=join_level);

        //Since we know they are prime absorbtion material, we just do it and
        //mark them as children. There's one subtlety here: We may have to
        //revise our search status once they absorb!! That's TODO: test to see
        //if we adequately handle premature absorb requests. After all, our
        //MWOE might now be in their subtree. 
        set_edge_status(join_root, MST);
        
        //Anyway, because we aren't in the initiating partition, the other guy
        //already has us marked MST, so we don't need to do anything.  -- a
        //leader somewhere else will send the next round start (or perhaps it
        //will be one of us when we do a merge()
        qsz=0;
        return GHS_OK;
      }   
    } else {
      assert(false && "unexpected library error: could not absorb / merge in 'join_us' processing b/c of unexpected edge type between partitions");
      return GHS_ERR_IMPL;
    }

  assert(false && "unexpected library error: reached end of function somehow ");
  return GHS_ERR_IMPL;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_noop(StaticQueue<Msg,BUF_SZ> &buf, size_t &qsz){
  algorithm_converged=true;
  return mst_broadcast(MsgType::NOOP, {},buf, qsz);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::typecast(const EdgeStatus status, const MsgType m, const MsgData &data, StaticQueue<Msg,BUF_SZ> &buf, size_t &qsz)const noexcept{
  size_t sent=0;
  for (size_t idx=0;idx<n_peers;idx++){
    const Edge &e = outgoing_edges[idx];
    assert(e.root==my_id && "Had an edge in outgoing_edges that was not rooted on me!");
    if ( e.status == status ){
      sent++;
      Msg to_send;
      to_send.type = m;
      to_send.to=e.peer;
      to_send.from=my_id;
      to_send.data = data;
      buf.push( to_send );
    }
  }
  qsz=sent;
  return GHS_OK;
}


template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::mst_broadcast(const MsgType m, const MsgData &data, StaticQueue<Msg,BUF_SZ> &buf, size_t&qsz)const noexcept{
  size_t sent =0;
  for (size_t idx =0;idx<n_peers;idx++){
    const Edge&e = outgoing_edges[idx];
    assert(e.root==my_id && "Had an edge in outgoing_edges that was not rooted on me!");
    if (e.status == MST && e.peer != parent){
      sent++;
      Msg to_send;
      to_send.type = m;
      to_send.to=e.peer;
      to_send.from=my_id;
      to_send.data = data;
      buf.push( to_send );
    }
  }
  qsz=sent;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::mst_convergecast(const MsgType m, const MsgData& data, StaticQueue<Msg,BUF_SZ> &buf, size_t &qsz)const noexcept{
  size_t sent=0;
  for (size_t idx =0;idx<n_peers;idx++){
    const Edge&e = outgoing_edges[idx];
    assert(e.root==my_id && "Had an edge in outgoing_edges that was not rooted on me!");
    if (e.status == MST && e.peer == parent){
      sent++;
      Msg to_send;
      to_send.type = m;
      to_send.to=e.peer;
      to_send.from=my_id;
      to_send.data = data;
      buf.push( to_send );
    }
  }
  qsz=sent;
  return GHS_OK;
}


template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_parent_id(const AgentID& id) noexcept{

  //case 1: self-loop ok
  if (id==get_id()){
    parent = id; 
    return GHS_OK;
  }

  //case 2: MST links ok
  if (has_edge(id) && get_edge(id).status == MST ){
    parent = id; 
    return GHS_OK;
  }
  return GHS_ERR_NO_SUCH_PARENT;
}


template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
AgentID GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_parent_id() const noexcept{
  return parent;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
AgentID GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_leader_id() const noexcept{
  return my_leader;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
void GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_leader_id(const AgentID& leader) noexcept{
  my_leader = leader;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
AgentID GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_level() const noexcept{
  return my_level;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
void GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_level(const Level & l){
  my_level = l;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
bool GhsState<GHS_MAX_AGENTS, BUF_SZ>::is_converged() const noexcept{
  return algorithm_converged;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
bool GhsState<GHS_MAX_AGENTS, BUF_SZ>::index_of(const AgentID& who, size_t &idx) const{
  for (size_t i=0;i<n_peers;i++){
    if (peers[i] == who){
      idx = i;
      return true;
    }
  }
  return false;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_waiting_for(const AgentID &who, bool waiting){
  size_t idx=0;
  GhsError found = index_of(who,idx);
  
  if (GhsOK(found)) {
    waiting_for_response[idx]=waiting;
    return GHS_OK;
  }
  assert (false && "Could not find peer: "+who);
  return GHS_NO_SUCH_PEER;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::is_waiting_for(const AgentID& who, bool &waiting){
  size_t idx=0;
  GhsError found = index_of(who,idx);
  if (GhsOK(found)) {
    waiting = waiting_for_response[idx];
    return GHS_OK;
  }
  assert (false && "Could not find peer: "+who);
  return GHS_NO_SUCH_PEER;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_response_required(const AgentID &who, bool resp)
{
  size_t idx=0;
  GhsError found= index_of(who,idx);
  if (GhsOK(found)){
    response_required[idx]=resp;
    return GHS_OK;
  }

  assert (false && "Could not find peer: "+who);
  return GHS_NO_SUCH_PEER;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::is_response_required(const AgentID &who, bool & res_req){
  size_t idx=0;
  GhsError found = index_of(who,idx);
  if (GhsOK(found)){
    res_req = response_required[idx];
    return GHS_OK;
  }
  assert (false && "Could not find peer: "+who);
  return GHS_NO_SUCH_PEER;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_response_prompt(const AgentID &who, const InPartPayload& m){
  size_t idx=0;
  GhsError found = index_of(who,idx);
  if (GhsOK(found)){
    response_prompt[idx]=m;
    return GHS_OK;
  }
  assert (index_of(who,idx) && "Could not find peer: "+who);
  return GHS_NO_SUCH_PEER;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>:: get_response_prompt(const AgentID &who, InPartPayload &out){
  size_t idx=0;
  GhsError found = index_of(who,idx);
  if (GhsOK(found)){
  }
  assert (index_of(who,idx) && "Could not find peer: "+who);
  return response_prompt[idx];
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
bool GhsState<GHS_MAX_AGENTS, BUF_SZ>::has_edge(const AgentID& to) const{
  size_t idx=0;
  return index_of(to,idx);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
Edge GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_edge(const AgentID& to)  const
{
  size_t idx=0;
  assert (index_of(to,idx) && "Could not find peer: "+to);
  return outgoing_edges[idx];
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>

void GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_edge_status(const AgentID &to, const EdgeStatus &status)
{
  size_t idx=0;
  assert (index_of(to,idx) && "Could not find peer: "+to);
  outgoing_edges[idx].status=status;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
void GhsState<GHS_MAX_AGENTS, BUF_SZ>::add_edge_to(const AgentID& who ) {
  Edge e;
  e.peer=who;
  e.root=my_id;
  e.status=UNKNOWN;
  e.metric_val=0;
  add_edge(e);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
int GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_edge(const Edge &e) {

  if (e.root != my_id){
    throw std::invalid_argument("Cannot add an edge that is not rooted on current node");
  }

  size_t idx=0;
  AgentID who = e.peer;
  if (index_of(who,idx)){
    outgoing_edges[idx].metric_val  =  e.metric_val;
    outgoing_edges[idx].status      =  e.status;
    return 0;
  } else {
    //if we got this far, we didn't find the peer yet. 
    assert(n_peers<GHS_MAX_AGENTS);
    peers[n_peers]=e.peer;
    outgoing_edges[n_peers] = e;
    n_peers++;
    return 1;
  }
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
size_t GhsState<GHS_MAX_AGENTS, BUF_SZ>::waiting_count() const noexcept
{
  int waiting =0;
  //count them up. 
  for (size_t i=0;i<n_peers;i++){
    if (waiting_for_response[i]){
      waiting++;
    }
  }
  return waiting;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
size_t GhsState<GHS_MAX_AGENTS, BUF_SZ>::delayed_count() const noexcept
{
  int delayed =0;
  for (size_t i=0;i<n_peers;i++){
    if (response_required[i]){
      delayed++;
    }
  }
  return delayed;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
AgentID GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_id() const noexcept {
  return my_id;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
void GhsState<GHS_MAX_AGENTS, BUF_SZ>::respond_later(const AgentID&from, const InPartPayload &m)
{
  int idx=0;
  bool found = index_of(from,idx);
  if (!found){
    assert(false&&"Could not find agent that we are trying to save message from!");
  }
  response_required[idx]=true;
  response_prompt[idx]  =m;
}
