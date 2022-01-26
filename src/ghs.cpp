#include "ghs.hpp"
#include "msg.hpp"
#include <algorithm>
#include <assert.h>
#include <deque>
#include <stdexcept>
#include <sstream>
#include <string>
#include <optional> //req c++20

using std::max;

GhsState::GhsState(AgentID my_id) noexcept{
  this->my_id =  my_id;
  reset();
}

GhsState::~GhsState(){}

/**
 * Reset the algorithm status completely
 */
bool GhsState::reset() noexcept{
  set_leader_id(my_id);
  set_level(0);
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

  return true;
}


/**
 * Queue up the start of the round
 *
 */
size_t GhsState::start_round(StaticQueue<Msg> &outgoing_buffer) noexcept{
  //If I'm leader, then I need to start the process. Otherwise wait
  if (my_leader == my_id){
    //nobody tells us what to do but ourselves
    return process_srch(my_id, {my_leader, my_level}, outgoing_buffer);
  }
  return 0;
}

Edge GhsState::mwoe() const noexcept{
  return best_edge;
}

size_t GhsState::process(const Msg &msg, StaticQueue<Msg> &outgoing_buffer){
  if (msg.from == my_id ){
    throw std::invalid_argument("Got a message from ourselves");
  }
  if (msg.to != my_id){
    throw std::invalid_argument("Tried to process message that was not for me");
  }
  switch (msg.type){
    case    (MsgType::SRCH):{         return  process_srch(         msg.from, msg.data.srch, outgoing_buffer);  }
    case    (MsgType::SRCH_RET):{     return  process_srch_ret(     msg.from, msg.data.srch_ret, outgoing_buffer);  }
    case    (MsgType::IN_PART):{      return  process_in_part(      msg.from, msg.data.in_part, outgoing_buffer);  }
    case    (MsgType::ACK_PART):{     return  process_ack_part(     msg.from, msg.data.ack_part, outgoing_buffer);  }
    case    (MsgType::NACK_PART):{    return  process_nack_part(    msg.from, msg.data.nack_part, outgoing_buffer);  }
    case    (MsgType::JOIN_US):{      return  process_join_us(      msg.from, msg.data.join_us, outgoing_buffer);  }
    case    (MsgType::NOOP):{         return  process_noop( outgoing_buffer ); }
    //case    (MsgType::ELECTION):{     return  process_election(     msg.from, msg.data, outgoing_buffer);  }
    //case    (MsgType::NOT_IT):{       return  process_not_it(       this,msg.from, msg.data);      }
    default:{ throw std::invalid_argument("Got unrecognzied message"); }
  }
  return true;
}

size_t GhsState::process_srch(  AgentID from, const SrchPayload& data, StaticQueue<Msg>&buf)
{
  //we either sent to ourselves, OR we should have an edge to them
  if (!has_edge(from) && from!=my_id){
    throw std::invalid_argument("Unknown sender");
  }
  
  if (from !=my_id && get_edge(from).status !=MST){
    throw std::invalid_argument("Invalid sender");
  }

  assert(from == my_id ||  get_edge(from).status == MST); // now that would be weird if it wasn't

  if (this->waiting_count() != 0 )
  {
    throw std::invalid_argument(" Waiting but got SRCH ");
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
  StaticQueue<Msg> srchbuf;

  //first broadcast
  MsgData to_send;
  to_send.srch = SrchPayload{my_leader, my_level};
  size_t srch_sent = mst_broadcast(MsgType::SRCH, to_send, srchbuf);

  //then ping unknown edges
  //OPTIMIZATION: Ping neighbors in sorted order, rather than flooding

  to_send.in_part = InPartPayload{my_leader, my_level};
  size_t part_sent = typecast(EdgeStatus::UNKNOWN, MsgType::IN_PART, to_send, srchbuf);

  //remember who we sent to so we can wait for them:
  size_t srchbuf_sz = srchbuf.size();
  size_t buf_sz = buf.size();
  assert(srchbuf_sz == srch_sent + part_sent);

  for (size_t i=0;i<srchbuf_sz;i++){
    Msg m;
    srchbuf.pop(m);
    set_waiting_for(m.to, true);
    buf.push(m);
  }

  assert( (buf.size() - buf_sz == srch_sent + part_sent)  );

  //make sure to check_new_level, since our level may have changed, above. 
  return srch_sent + part_sent + check_new_level(buf);
}

size_t GhsState::process_srch_ret(  AgentID from, const SrchRetPayload &data, StaticQueue<Msg>&buf)
{

  if (this->waiting_count() == 0){
    throw std::invalid_argument("Got a SRCH_RET but we aren't waiting for anyone -- did we reset()?");
  }

  if (! is_waiting_for(from) )
  {
    throw std::invalid_argument("Got a SRCH_RET from node we aren't waiting_for");
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

  return check_search_status(buf);
}

size_t GhsState::process_in_part(  AgentID from, const InPartPayload& data, StaticQueue<Msg>&buf)
{
  //let them know if we're in their partition or not. Easy.
  size_t part_id = data.leader;

  //except if they are *ahead* of us in the execution of their algorithm. That is, what if we
  //don't actually know if we are in their partition or not? This is detectable if their level > ours. 
  size_t their_level   = data.level;
  size_t our_level     = my_level;

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
      return 1;
    } else {
      Msg to_send = NackPartPayload{}.to_msg(from, my_id);
      buf.push (to_send); 
      return 1;
    } 
  } else {
    respond_later(from, data);
    return 0;
  }
}

size_t GhsState::process_ack_part(  AgentID from, const AckPartPayload& data, StaticQueue<Msg>&buf)
{
  //we now know that the sender is in our partition. Mark their edge as deleted
  if (!is_waiting_for(from))
  {
    throw std::invalid_argument("We got a IN_PART message from "+std::to_string(from)+" but we weren't waiting for one");
  }
  set_edge_status(from, DELETED);
  set_waiting_for(from, false);
  return check_search_status(buf);
}

size_t GhsState::process_nack_part(  AgentID from, const NackPartPayload &data, StaticQueue<Msg>&buf)
{
  //we now know that the sender is in our partition. Mark their edge as deleted
  if (!is_waiting_for(from)){
    throw std::invalid_argument("We got a IN_PART message from "+std::to_string(from)+" but we weren't waiting for one");
  }
  assert(has_edge(from));
  Edge their_edge = get_edge( from );
  
  if (best_edge.metric_val > their_edge.metric_val){
    best_edge = their_edge;
  }

  //@throws:
  set_waiting_for(from,false);
  return check_search_status(buf);
}

size_t GhsState::check_search_status(StaticQueue<Msg> &buf){
  
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
      return mst_convergecast( MsgType::SRCH_RET, send_data, buf);
    }

    if (am_leader && found_new_edge && its_my_edge){
      //just start the process to join up, rather than sending messages
      assert(e.peer != e.root); //ask me why I check

      return process_join_us(my_id, {e.peer, e.root, get_leader_id(), get_level()}, buf);
    }

    if (am_leader && !found_new_edge ){
      //I'm leader, no new edge, let's move on b/c we're done here
      return process_noop( buf );
    }

    if (am_leader && found_new_edge && !its_my_edge){
      //inform the crew to add the edge
      auto msg = JoinUsPayload{e.peer, e.root, get_leader_id(), get_level()}.to_msg(0,0);
      return mst_broadcast( msg.type, msg.data, buf);
    }
  } 

  return 0;
}


size_t GhsState::check_new_level( StaticQueue<Msg> &buf){

  size_t ret=0;
  for (size_t idx=0;idx<n_peers;idx++){
    if (response_required[idx]){
      AgentID who = peers[idx];
      InPartPayload &m = response_prompt[idx];
      Level their_level = m.level; 
      if (their_level <= get_level() )
      {
        //ok to answer, they were waiting for us to catch up
        ret+=process_in_part(who, m, buf);
        response_required[idx]=false;
      } 
    }
  }

  return ret;
}


size_t GhsState::process_join_us(  AgentID from, const JoinUsPayload &data, StaticQueue<Msg>&buf)
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
      throw std::invalid_argument("We should never receive a JOIN_US with a different leader, unless we are on the partition boundary");
    }
    if (join_level != my_level){
      throw std::invalid_argument("We should never receive a JOIN_US with a different level, unless we are on the partition boundary");
    }  

    MsgData to_send;
    to_send.join_us = data;
    return mst_broadcast(MsgType::JOIN_US, to_send, buf);
  }

  Edge edge_to_other_part;

  if (in_initiating_partition){
    //leader CAN be different, even though we're initating, IF we're on an MST
    //link to them
    assert(has_edge(join_peer));
    Edge join_peer_edge = get_edge(join_peer);

    if (join_lead != my_leader && join_peer_edge.status != MST){
      //std::cerr<<"My leader: "<<my_leader<<std::endl;
      //std::cerr<<"join_lead: "<<join_lead<<std::endl;
      throw std::invalid_argument("We should never receive a JOIN_US with a different leader when we initiate");
    }
    if (join_level != my_level){
      throw std::invalid_argument("We should never receive a JOIN_US with a different level when we initiate");
    }  
    assert(has_edge(join_peer));
    edge_to_other_part  = get_edge(join_peer);
  } else {
    if (join_lead == my_leader){
      throw std::invalid_argument("We should never receive a JOIN_US with same leader from a different partition");
    }
    //level can be same, lower (from another partition), but not higher (we shouldn't have replied)
    if (join_level > my_level){
      throw std::invalid_argument("We should never receive a JOIN_US with a higher level when we do not initiate (we shouldnt have replied to their IN_PART)");
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
      return start_round(buf);

    } else {
      //In this case, we already sent JOIN_US (b/c it's an MST link), meaning
      //this came from THEM. If they rec'd ours first, then they know if they
      //are leader or not. if not, AND we're not leader, we're at deadlock
      //until they process our JOIN_US and see the MST link from their JOIN_US
      //request and recognize their own leader-hood. We wait. 
      return 0;
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
        return 1;
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
        return 0;
      }   
    } else {
      assert(false && "unexpected library error: could not absorb / merge in 'join_us' processing b/c of unexpected edge type between partitions");
    }

  assert(false && "unexpected library error: reached end of function somehow ");
  return 0;
}

size_t GhsState::process_noop(StaticQueue<Msg> &buf){
  algorithm_converged=true;
  return mst_broadcast(MsgType::NOOP, {},buf);
}

size_t GhsState::typecast(const EdgeStatus status, const MsgType m, const MsgData &data, StaticQueue<Msg> &buf)const noexcept{
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
  return sent;
}


size_t GhsState::mst_broadcast(const MsgType m, const MsgData &data, StaticQueue<Msg> &buf)const noexcept{
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
  return sent;
}

size_t GhsState::mst_convergecast(const MsgType m, const MsgData& data, StaticQueue<Msg> &buf)const noexcept{
  size_t sent(0);
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
  return sent;
}


void GhsState::set_parent_id(const AgentID& id){

  //case 1: self-loop ok
  if (id==get_id()){
    parent = id; 
    return;
  }

  //case 2: MST links ok
  assert( has_edge(id) );
  assert( get_edge(id).status == MST );
  parent = id; 

}
AgentID GhsState::get_parent_id() const noexcept{
  return parent;
}

AgentID GhsState::get_leader_id() const noexcept{
  return my_leader;
}

void GhsState::set_leader_id(const AgentID& leader){
  my_leader = leader;
}

AgentID GhsState::get_level() const noexcept{
  return my_level;
}

void GhsState::set_level(const Level & l){
  my_level = l;
}

bool GhsState::is_converged() const noexcept{
  return algorithm_converged;
}

bool GhsState::index_of(const AgentID& who, size_t &idx) const{
  for (size_t i=0;i<n_peers;i++){
    if (peers[i] == who){
      idx = i;
      return true;
    }
  }
  return false;
}

void GhsState::set_waiting_for(const AgentID &who, bool waiting){
  size_t idx=0;
  assert (index_of(who,idx) && "Could not find peer: "+who);
  waiting_for_response[idx]=waiting;
}

bool GhsState::is_waiting_for(const AgentID& who){
  size_t idx=0;
  assert (index_of(who,idx) && "Could not find peer: "+who);
  return waiting_for_response[idx];
}

void GhsState::set_response_required(const AgentID &who, bool resp)
{
  size_t idx=0;
  assert (index_of(who,idx) && "Could not find peer: "+who);
  response_required[idx]=resp;
}

bool GhsState::is_response_required(const AgentID &who){
  size_t idx=0;
  assert (index_of(who,idx) && "Could not find peer: "+who);
  return response_required[idx];
}

void GhsState::set_response_prompt(const AgentID &who, const InPartPayload& m){
  size_t idx=0;
  assert (index_of(who,idx) && "Could not find peer: "+who);
  response_prompt[idx]=m;
}

InPartPayload GhsState:: get_response_prompt(const AgentID &who){
  size_t idx=0;
  assert (index_of(who,idx) && "Could not find peer: "+who);
  return response_prompt[idx];
}

bool GhsState::has_edge(const AgentID& to) const{
  size_t idx=0;
  return index_of(to,idx);
}

Edge GhsState::get_edge(const AgentID& to)  const
{
  size_t idx=0;
  assert (index_of(to,idx) && "Could not find peer: "+to);
  return outgoing_edges[idx];
}


void GhsState::set_edge_status(const AgentID &to, const EdgeStatus &status)
{
  size_t idx=0;
  assert (index_of(to,idx) && "Could not find peer: "+to);
  outgoing_edges[idx].status=status;
}

size_t GhsState::set_edge(const Edge &e) {

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

size_t GhsState::waiting_count() const noexcept
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

size_t GhsState::delayed_count() const noexcept
{
  size_t delayed =0;
  for (size_t i=0;i<n_peers;i++){
    if (response_required[i]){
      delayed++;
    }
  }
  return delayed;
}

AgentID GhsState::get_id() const noexcept {
  return my_id;
}

void GhsState::respond_later(const AgentID&from, const InPartPayload &m)
{
  size_t idx;
  assert( index_of(from,idx) );
  response_required[idx]=true;
  response_prompt[idx]  =m;
}
