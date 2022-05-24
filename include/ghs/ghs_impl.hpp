#include "ghs.hpp"
#include "msg.hpp"

#ifndef NDEBUG
#include <stdexcept>
#define ghs_fatal(s) throw std::runtime_error(std::to_string(__LINE__)+":"+ghs_strerror(s))
#define ghs_fatal_msg(s,m) throw std::runtime_error(std::to_string(__LINE__)+":"+ghs_strerror(s)+"--"+std::to_string(m))
#else
#define ghs_fatal(__VA_ARGS__) if(false){}
#define ghs_fatal_msg(__VA_ARGS__) if (false) {}
#endif

using std::max;

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsState<GHS_MAX_AGENTS, BUF_SZ>::GhsState(AgentID my_id) {
  this->my_id =  my_id;
  reset();
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsState<GHS_MAX_AGENTS, BUF_SZ>::~GhsState(){}

/**
 * Reset the algorithm status completely
 */
template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::reset() {
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
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::start_round(StaticQueue<Msg,BUF_SZ> &outgoing_buffer, size_t & qsz) {
  //If I'm leader, then I need to start the process. Otherwise wait.
  if (my_leader == my_id){
    //nobody tells us what to do but ourselves
    return process_srch(my_id, {my_leader, my_level}, outgoing_buffer, qsz);
  }
  qsz=0;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
Edge GhsState<GHS_MAX_AGENTS, BUF_SZ>::mwoe() const {
  return best_edge;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process(const Msg &msg, StaticQueue<Msg,BUF_SZ> &outgoing_buffer, size_t &qsz) {

  if (msg.from==my_id){
    //ghs_debug_crash("Received msg from self!");
    return GHS_PROCESS_SELFMSG;
  }

  if (msg.to != my_id){
    return GHS_PROCESS_NOTME;
  }

  if (! has_edge(msg.from) ){
    return GHS_PROCESS_NO_EDGE_FOUND;
  }

  switch (msg.type){
    case    (MsgType::SRCH):{         return  process_srch(         msg.from, msg.data.srch, outgoing_buffer, qsz);  }
    case    (MsgType::SRCH_RET):{     return  process_srch_ret(     msg.from, msg.data.srch_ret, outgoing_buffer, qsz);  }
    case    (MsgType::IN_PART):{      return  process_in_part(      msg.from, msg.data.in_part, outgoing_buffer, qsz);  }
    case    (MsgType::ACK_PART):{     return  process_ack_part(     msg.from, msg.data.ack_part, outgoing_buffer, qsz);  }
    case    (MsgType::NACK_PART):{    return  process_nack_part(    msg.from, msg.data.nack_part, outgoing_buffer, qsz);  }
    case    (MsgType::JOIN_US):{      return  process_join_us(      msg.from, msg.data.join_us, outgoing_buffer, qsz);  }
    case    (MsgType::NOOP):{         return  process_noop(         outgoing_buffer , qsz); }
    default:{ return GHS_PROCESS_INVALID_TYPE; }
  }
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_srch(  AgentID from, const SrchPayload& data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{

  //this msg is weird, in that we sometimes trigger internally with from==my_id

  if (from!=my_id){//must be MST edge then
    Edge to_them;
    auto ret = get_edge(from, to_them);
    if (ret != GHS_OK) 
    { 
      return ret; 
    }
    if (to_them.status!=MST ) 
    { 
      return GHS_PROCESS_REQ_MST; 
    }
  }

  if (waiting_count() != 0 )
  {
    return GHS_SRCH_STILL_WAITING;
  }

  //grab the new partition information, since only one node / partition sends srch() msgs.
  AgentID leader = data.your_leader;
  Level   level  = data.your_level;
  my_leader = leader;
  my_level  = level;
  //also note our parent may have changed
  parent=from;

  //initialize the best edge to a bad value for comparisons
  best_edge = ghs_worst_possible_edge();
  best_edge.root = my_id;

  //we'll cache outgoing messages temporarily
  StaticQueue<Msg,BUF_SZ> srchbuf;

  //first broadcast the SRCH down the tree
  MsgData to_send;
  to_send.srch = SrchPayload{my_leader, my_level};
  size_t srch_sent=0;
  GhsError srch_ret = mst_broadcast(MsgType::SRCH, to_send, srchbuf,srch_sent);
  if (srch_ret!=GHS_OK){
    return srch_ret;
  }

  //then ping unknown edges
  //OPTIMIZATION: Ping neighbors in sorted order, rather than flooding

  to_send.in_part = InPartPayload{my_leader, my_level};
  size_t part_sent=0;
  GhsError part_ret = typecast(EdgeStatus::UNKNOWN, MsgType::IN_PART, to_send, srchbuf, part_sent);
  if (part_ret!=GHS_OK){
    return part_ret;
  }

  //remember who we sent to so we can wait for them:
  size_t srchbuf_sz = srchbuf.size();
  if (srchbuf_sz != srch_sent + part_sent){
    return GHS_ERR_QUEUE_MSGS;
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
    set_waiting_for(m.to, true);
    buf.push(m);
  }

  if ( (buf.size() - buf_sz_before != srch_sent + part_sent )  )
  {
    return GHS_ERR_QUEUE_MSGS;
  }

  //make sure to check_new_level, since our level may have changed, above,
  //which will handled delayed_count != 0;
  size_t old_msgs_processed=0;
  GhsError lvl_err = check_new_level(buf,old_msgs_processed);
  if (lvl_err != GHS_OK ){
    return lvl_err;
  }

  //notify hunky dory
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
    return GHS_UNEXPECTED_SRCH_RET;
  }

  bool wf=false;
  auto wfr = is_waiting_for(from,wf);
  if (GHS_OK!=wfr){
    return wfr;
  }
  if ( !wf ){
    return GHS_UNEXPECTED_SRCH_RET;
  }

  auto swfr = set_waiting_for(from,false);
  if (GHS_OK != swfr ){
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

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_in_part(  AgentID from, const InPartPayload& data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{
  //let them know if we're in their partition or not. Easy.
  AgentID part_id = data.leader;

  //except if they are *ahead* of us in the execution of their algorithm. That is, what if we
  //don't actually know if we are in their partition or not? This is detectable if their level > ours. 
  Level their_level   = data.level;
  Level our_level     = my_level;

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
  //is this the right time to receive this msg?
  bool wf=false; 
  auto wfr = is_waiting_for(from,wf);
  if (GHS_OK!=wfr){
    return wfr;
  }
  if (!wf)
  {
    return GHS_ACK_NOT_WAITING;
  }

  //we now know that the sender is in our partition. Mark their edge as deleted
  auto sesr = set_edge_status(from, DELETED);
  if (GHS_OK != sesr){
    return sesr;
  }

  auto swfr = set_waiting_for(from, false);
  if (GHS_OK != swfr){
    return swfr;
  }

  return check_search_status(buf,qsz);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_nack_part(  AgentID from, const NackPartPayload &data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{
  //is this the right time to receive this msg?
  bool wf=false; 
  auto wfr = is_waiting_for(from,wf);
  if (GHS_OK!=wfr){
    return wfr;
  }
  if (!wf)
  {
    return GHS_ACK_NOT_WAITING;
  }

  Edge their_edge;
  //although has_edge was checked, we are careful anyway
  auto ger = get_edge(from, their_edge);
  if (GHS_OK != ger)
  {
    return GHS_PROCESS_NO_EDGE_FOUND;
  }
  
  if (best_edge.metric_val > their_edge.metric_val){
    best_edge = their_edge;
  }

  auto swfr = set_waiting_for(from,false);
  if (GHS_OK != swfr){
    return swfr;
  }

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
      if (e.peer == e.root){
        return GHS_BAD_MSG;
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
      //awkward way to construct a MsgData struct, but we do it this way anyway. See to_msg(x,y) in msg.hpp
      auto msg = JoinUsPayload{e.peer, e.root, get_leader_id(), get_level()}.to_msg(0,0);
      return mst_broadcast( msg.type, msg.data, buf, qsz);
    }
  } 

  //nothing to do
  qsz=0;
  return GHS_OK;
}


template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::check_new_level( StaticQueue<Msg,BUF_SZ> &buf, size_t & qsz){

  qsz=0;
  for (size_t idx=0;idx<n_peers;idx++){
    if (response_required[idx]){
      AgentID who = peers[idx];
      InPartPayload &m = response_prompt[idx];
      Level their_level = m.level; 
      if (their_level <= get_level() )
      {
        size_t sentsz=0;
        //ok to answer, they were waiting for us to catch up
        GhsError ret=process_in_part(who, m, buf, sentsz);
        if (ret!=GHS_OK){
          //some error, propegate up
          return ret;
        }
        qsz+=sentsz;
        response_required[idx]=false;
      } 
    }
  }
  return GHS_OK;
}


template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_join_us(  AgentID from, const JoinUsPayload &data, StaticQueue<Msg,BUF_SZ>&buf, size_t & qsz)
{

  auto join_peer  = data.join_peer; // the side of the edge that is in the other partition 
  auto join_root  = data.join_root; // the side of the edge that is in the partition initiating join
  auto join_lead  = data.proposed_leader; // the leader, as declared during search, of the peer
  auto join_level = data.proposed_level; // the level of the peer as declared during search

  bool not_involved            = (join_root != my_id && join_peer != my_id);
  bool in_initiating_partition = (join_root == my_id);

  if (not_involved){
    if (join_lead != my_leader){
      return GHS_JOIN_BAD_LEADER;
    }
    if (join_level != my_level){
      return GHS_JOIN_BAD_LEVEL;
    }  

    MsgData to_send;
    to_send.join_us = data;
    return mst_broadcast(MsgType::JOIN_US, to_send, buf, qsz);
  }

  //let's find the edge to the other partition
  Edge edge_to_other_part;

  if (in_initiating_partition){
    //leader CAN be different, even though we're initating, IF we're on an MST
    //link to them
    GhsError retcode;
    Edge join_peer_edge;
    retcode = get_edge(join_peer, join_peer_edge);
    if (retcode != GHS_OK){
      return retcode;
    }

    //check join_peer status (better be MST)
    if (join_lead != my_leader && join_peer_edge.status != MST){
      return GHS_JOIN_INIT_BAD_LEADER;
    }
    if (join_level != my_level){
      return GHS_JOIN_INIT_BAD_LEVEL;
    }  
    //found the correct edge
    retcode = get_edge(join_peer, edge_to_other_part);
    if (GHS_OK != retcode)
    { 
      return retcode; 
    }
  } else {
    //we aren't in initiating partition, and yet it includes our partition, this is a problem!
    if (join_lead == my_leader){
      return GHS_JOIN_MY_LEADER;
    }
    //level can be same, lower (from another partition), but not higher (we shouldn't have replied)
    if (join_level > my_level){
      return GHS_JOIN_UNEXPECTED_REPLY;
    }  
    //found the correct edge
    auto ger = get_edge(join_root, edge_to_other_part);
    if (GHS_OK != ger)
    { 
      //ghs_fatal_msg(ger,join_root);
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
        Msg to_send = JoinUsPayload{
          data.join_peer,
            data.join_root,
            data.proposed_leader,
            data.proposed_level
        }.to_msg(join_peer,my_id);
        buf.push(to_send);
        qsz=1;
        return GHS_OK;
      } else {

        //In this case, we received a JOIN_US from another partition, one that
        //we have not yet recognized or marked as our own MWOE. This means they
        //are a prime candidate to absorb into our partition. 
        //NOTE, if we were waiting for them, they would not respond until their
        //level is == ours, so this should never fail:
        if (my_level < join_level){
          return GHS_JOIN_UNEXPECTED_REPLY;
        }

        //Since we know they are prime absorbtion material, we just do it and
        //mark them as children. There's one subtlety here: We may have to
        //revise our search status once they absorb!! That's TODO: test to see
        //if we adequately handle premature absorb requests. After all, our
        //MWOE might now be in their subtree. 
        auto sesr = set_edge_status(join_root, MST);
        if (GHS_OK != sesr){
          return sesr;
        }
        
        //Anyway, because we aren't in the initiating partition, the other guy
        //already has us marked MST, so we don't need to do anything.  -- a
        //leader somewhere else will send the next round start (or perhaps it
        //will be one of us when we do a merge()
        qsz=0;
        return GHS_OK;
      }   
    } else {
      ghs_fatal(GHS_ERR_IMPL);
      return GHS_ERR_IMPL;
    }

  ghs_fatal(GHS_ERR_IMPL);
  return GHS_ERR_IMPL;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::process_noop(StaticQueue<Msg,BUF_SZ> &buf, size_t &qsz){
  algorithm_converged=true;
  return mst_broadcast(MsgType::NOOP, {},buf, qsz);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::typecast(const EdgeStatus status, const MsgType m, const MsgData &data, StaticQueue<Msg,BUF_SZ> &buf, size_t &qsz)const {
  size_t sent=0;
  for (size_t idx=0;idx<n_peers;idx++){
    const Edge &e = outgoing_edges[idx];
    if (e.root!=my_id){
      return GHS_CAST_INVALID_EDGE;
    }
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
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::mst_broadcast(const MsgType m, const MsgData &data, StaticQueue<Msg,BUF_SZ> &buf, size_t&qsz)const {
  size_t sent =0;
  for (size_t idx =0;idx<n_peers;idx++){
    const Edge&e = outgoing_edges[idx];
    if (e.root!=my_id){
      return GHS_CAST_INVALID_EDGE;
    }
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
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::mst_convergecast(const MsgType m, const MsgData& data, StaticQueue<Msg,BUF_SZ> &buf, size_t &qsz)const {
  size_t sent=0;
  for (size_t idx =0;idx<n_peers;idx++){
    const Edge&e = outgoing_edges[idx];
    if (e.root!=my_id){
      return GHS_CAST_INVALID_EDGE;
    }
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
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_parent_id(const AgentID& id) {

  //case 1: self-loop ok
  if (id==get_id()){
    parent = id; 
    return GHS_OK;
  }

  //case 2: MST links ok
  if (!has_edge(id)){
    return GHS_PARENT_UNRECOGNIZED;
  }
  Edge peer;
  auto ger = get_edge(id,peer);
  if (GHS_OK != ger){
    return ger;
  }
  if (peer.status == MST ){
    parent = id; 
    return GHS_OK;
  }
  return GHS_PARENT_REQ_MST;
}


template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
AgentID GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_parent_id() const {
  return parent;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
AgentID GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_leader_id() const {
  return my_leader;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_leader_id(const AgentID& leader) {
  my_leader = leader;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
Level GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_level() const {
  return my_level;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_level(const Level & l){
  my_level = l;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
bool GhsState<GHS_MAX_AGENTS, BUF_SZ>::is_converged() const {
  return algorithm_converged;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::checked_index_of(const AgentID& who, size_t &idx) const{
  if (who == my_id){
    return GHS_IMPL_REQ_PEER_MY_ID;
  }
  for (size_t i=0;i<n_peers;i++){
    if (peers[i] == who){
      idx = i;
      return GHS_OK;
    }
  }
  return GHS_NO_SUCH_PEER;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_waiting_for(const AgentID &who, bool waiting){

  size_t idx;
  GhsError retcode=checked_index_of(who,idx);
  if (retcode!=GHS_OK){return retcode;}

  waiting_for_response[idx]=waiting;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::is_waiting_for(const AgentID& who, bool& waiting_for){
  size_t idx;
  GhsError retcode=checked_index_of(who,idx);
  if (retcode!=GHS_OK){return retcode;}

  waiting_for = waiting_for_response[idx];
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_response_required(const AgentID &who, bool resp)
{
  size_t idx;
  GhsError retcode=checked_index_of(who,idx);
  if (retcode!=GHS_OK){return retcode;}

  response_required[idx]=resp;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::is_response_required(const AgentID &who, bool &res_req ){
  size_t idx;
  GhsError retcode=checked_index_of(who,idx);
  if (retcode!=GHS_OK){return retcode;}

  res_req = response_required[idx];
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_response_prompt(const AgentID &who, const InPartPayload& m){
  size_t idx;
  GhsError retcode=checked_index_of(who,idx);
  if (retcode!=GHS_OK){return retcode;}

  response_prompt[idx]=m;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>:: get_response_prompt(const AgentID &who, InPartPayload &out){
  size_t idx;
  GhsError retcode=checked_index_of(who,idx);
  if (retcode!=GHS_OK){return retcode;}

  out = response_prompt[idx];
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::respond_later(const AgentID&from, const InPartPayload m)
{
  size_t idx;
  GhsError retcode=checked_index_of(from,idx);
  if (retcode!=GHS_OK){return retcode;}

  response_required[idx]=true;
  response_prompt[idx]  =m;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
bool GhsState<GHS_MAX_AGENTS, BUF_SZ>::has_edge(const AgentID& to) const{
  size_t idx;
  return GHS_OK==checked_index_of(to,idx);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_edge(const AgentID& to, Edge &out)  const
{
  size_t idx;
  GhsError retcode=checked_index_of(to,idx);
  if (retcode!=GHS_OK){return retcode;}

  out = outgoing_edges[idx];
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>

GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_edge_status(const AgentID &to, const EdgeStatus &status)
{
  size_t idx;
  GhsError retcode=checked_index_of(to,idx);
  if (retcode!=GHS_OK){return retcode;}

  outgoing_edges[idx].status=status;
  return GHS_OK;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::add_edge_to(const AgentID& who ) {
  Edge e;
  e.peer=who;
  e.root=my_id;
  e.status=UNKNOWN;
  e.metric_val=0;
  return add_edge(e);
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
GhsError GhsState<GHS_MAX_AGENTS, BUF_SZ>::set_edge(const Edge &e) {

  if (e.root != my_id){
    return GHS_SET_INVALID_EDGE;
  }

  AgentID who = e.peer;
  size_t idx;
  GhsError er = checked_index_of(who,idx);
  if (GHS_OK == er)
  {
    //found em
    outgoing_edges[idx].metric_val  =  e.metric_val;
    outgoing_edges[idx].status      =  e.status;
    return GHS_OK;
  } 
  else if (GHS_NO_SUCH_PEER == er)
  {
    //don't have em (yet)
    if (n_peers>=GHS_MAX_AGENTS){
      return GHS_TOO_MANY_AGENTS;
    }
    peers[n_peers]=e.peer;
    outgoing_edges[n_peers] = e;
    n_peers++;
    return GHS_OK;
  } 
  else 
  {
    //something else happened
    return er;
  }
  ghs_fatal(GHS_ERR_IMPL);
  return GHS_ERR_IMPL;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
size_t GhsState<GHS_MAX_AGENTS, BUF_SZ>::waiting_count() const 
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

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
size_t GhsState<GHS_MAX_AGENTS, BUF_SZ>::delayed_count() const 
{
  size_t delayed =0;
  for (size_t i=0;i<n_peers;i++){
    if (response_required[i]){
      delayed++;
    }
  }
  return delayed;
}

template <std::size_t GHS_MAX_AGENTS, std::size_t BUF_SZ>
AgentID GhsState<GHS_MAX_AGENTS, BUF_SZ>::get_id() const {
  return my_id;
}

