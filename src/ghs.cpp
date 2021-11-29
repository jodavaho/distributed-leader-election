#include "ghs.hpp"
#include "msg.hpp"
#include <algorithm>
#include <assert.h>
#include <deque>
#include <stdexcept>
#include <string>
#include <optional> //req c++20

GhsState::GhsState(AgentID my_id,  std::vector<Edge> edges) noexcept{
  this->my_id           =  my_id;
  this->my_part         =  Partition{my_id,0};
  this->parent          =  my_id; //I live alone
  this->waiting_for     =  {};
  this->respond_later   =  {};
  this->best_edge       =  ghs_worst_possible_edge();

  for (const auto e: edges){
    this->set_edge(e);
  }
}

/**
 * Reset the algorithm status completely
 */
bool GhsState::reset() noexcept{
  waiting_for.clear();
  respond_later.clear();
  this->parent = my_id;
  this->my_part = Partition{my_id,0};
  for (size_t i=0;i<outgoing_edges.size();i++){
    outgoing_edges[i].status=UNKNOWN;
  }
  return true;
}

std::optional<Edge> GhsState::get_edge(const AgentID& to) noexcept
{
  for (const auto edge: outgoing_edges){
    if (edge.peer == to){
      return edge;
    }
  }
  return std::nullopt;
}

AgentID GhsState::get_id() const noexcept {
  return my_id;
}

Partition GhsState::get_partition() const noexcept {
  return my_part; 
}

void GhsState::set_edge_status(const AgentID &to, const EdgeStatus &status)
{
  for (auto &edge:outgoing_edges){
    if (edge.peer == to){
       edge.status=status; 
       return;
    }
  }
  throw std::invalid_argument("Edge not found to "+std::to_string(to));
}

void GhsState::set_parent_edge(const Edge&e) {

  if (e.status != MST){
    throw std::invalid_argument("Cannot add non MST edge as parent!");
  }

  if (e.root != my_id){
    throw std::invalid_argument("Parent edge does not connect this node (my_id != e.root)!");
  }

  //if this is a new edge:
  if ( set_edge(e) > 0 )
  {
    throw std::invalid_argument("Cannot add new edge as parent! How would we have discovered parent during search if the edge was not known to us?");
  } 

  parent = e.peer;

}

size_t GhsState::set_edge(const Edge &e) {

  if (e.root != my_id){
    throw std::invalid_argument("Cannot add an edge that is not rooted on current node");
  }

  //Optimization: Use set, tree, or sorted agent ids to speed up this function
  //with large #s of agents.
  for (size_t i=0;i<outgoing_edges.size();i++){
    if (outgoing_edges[i].peer == e.peer)
    {
      outgoing_edges[i].metric_val  =  e.metric_val;
      outgoing_edges[i].status      =  e.status;
      return 0;
    }
  }
  //if we got this far, we didn't find the peer yet. 
  outgoing_edges.push_back(e);
  return 1;
}

/**
 * Set the partition for this guy/gal
 */
void GhsState::set_partition(const Partition &p) noexcept{
  my_part = p;
}

/**
 * Queue up the start of the round
 *
 */
void GhsState::start_round(std::deque<Msg> *outgoing_buffer) noexcept{
  //If I'm leader, then I need to start the process. Otherwise wait
  if (my_part.leader == my_id){
    //nobody tells us what to do but ourselves
    process_srch(my_id, {}, outgoing_buffer);
  }
}

size_t GhsState::waiting_count() const noexcept
{
  return waiting_for.size();
}

size_t GhsState::delayed_count() const noexcept
{
  return this->respond_later.size();
}

Edge GhsState::mwoe() const noexcept{
  return best_edge;
}

size_t GhsState::process(const Msg &msg, std::deque<Msg> *outgoing_buffer){
  if (msg.from == my_id ){
    throw std::invalid_argument("Got a message from ourselves");
  }
  if (msg.to != my_id){
    throw std::invalid_argument("Tried to process message that was not for me");
  }
  switch (msg.type){
    case    (Msg::Type::SRCH):{         return  process_srch(         msg.from, msg.data, outgoing_buffer);  }
    case    (Msg::Type::SRCH_RET):{     return  process_srch_ret(     msg.from, msg.data, outgoing_buffer);  }
    case    (Msg::Type::IN_PART):{      return  process_in_part(      msg.from, msg.data, outgoing_buffer);  }
    case    (Msg::Type::ACK_PART):{     return  process_ack_part(     msg.from, msg.data, outgoing_buffer);  }
    case    (Msg::Type::NACK_PART):{    return  process_nack_part(    msg.from, msg.data, outgoing_buffer);  }
    case    (Msg::Type::JOIN_US):{      return  process_join_us(      msg.from, msg.data, outgoing_buffer);  }
    case    (Msg::Type::ELECTION):{     return  process_election(     msg.from, msg.data, outgoing_buffer);  }
    case    (Msg::Type::NEW_SHERIFF):{  return  process_new_sheriff(  msg.from, msg.data, outgoing_buffer);  }
    //case    (Msg::Type::NOT_IT):{       return  process_not_it(       this,msg.from, msg.data);      }
    default:{ throw std::invalid_argument("Got unrecognzied message"); }
  }
  return true;
}

size_t GhsState::process_srch(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  if (from!=my_part.leader && from!=parent){
    //something is tragically wrong here, one of these must be true
    throw std::invalid_argument("Got a message from someone, not us, not our leader, and not our parent");
  }

  assert(waiting_for.size()==0 && " We got a srch msg while still waiting for results!");

  //we'll only receive this once, so we have to start the search when we get
  //this.

  //initialize the best edge to a bad value for comparisons
  best_edge = ghs_worst_possible_edge();
  best_edge.root = my_id;

  //process each edge differently
 
  //we'll cache outgoing messages temporarily
  std::deque<Msg> srchbuf;

  //first broadcast
  size_t srch_sent = mst_broadcast(Msg::Type::SRCH, {}, &srchbuf);

  //then ping unknown edges
  //OPTIMIZATION: Ping neighbors in sorted order, rather than flooding
  size_t part_sent = typecast(EdgeStatus::UNKNOWN, Msg::Type::IN_PART, {my_part.leader, my_part.level}, &srchbuf);

  //remember who we sent to so we can wait for them:
  assert(srchbuf.size() == srch_sent + part_sent);
  for (const auto& m: srchbuf){
    waiting_for.insert(m.to);
  }

  //queue up the messages
  size_t bufsz = buf->size();
  std::copy(srchbuf.begin(), srchbuf.end(), std::back_inserter(*buf));
  assert( (buf->size() -bufsz == srch_sent + part_sent)  );

  return srch_sent + part_sent;
}

size_t GhsState::process_srch_ret(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{

  if (waiting_for.size()==0){
    throw std::invalid_argument("Got a SRCH_RET but we aren't waiting for anyone -- did we reset()?");
  }

  if (waiting_for.find(from)==waiting_for.end())
  {
    throw std::invalid_argument("Got a SRCH_RET from node we aren't waiting_for");
  }

  waiting_for.erase(from);

  //compare our best edge to their best edge
  //
  //first, get their best edge
  //
  assert(data.size()==3);
  Edge theirs;
  theirs.peer        =  data[0];
  theirs.root        =  data[1];
  theirs.metric_val  =  data[2];

  if (theirs.metric_val < best_edge.metric_val){
    best_edge.root        =  theirs.root;
    best_edge.peer        =  theirs.peer;
    best_edge.metric_val  =  theirs.metric_val;
  }

  return check_search_status(buf);
}

size_t GhsState::process_in_part(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  //let them know if we're in their partition or not. Easy.
  assert(data.size()==2);
  size_t part_id = data[0];

  //except if they are *ahead* of us in the execution of their algorithm. That is, what if we
  //don't actually know if we are in their partition or not? This is detectable if their level > ours. 
  size_t their_level   = data[1];
  size_t our_level     = my_part.level;

  if (!get_edge(from)){
    //we don't have an edge to them. That's a warning condition, but not clear what to do. 
  }

  if (their_level <= our_level){
    //They aren't behind, so we can respond
    if (part_id == this->my_part.leader){
      buf->push_back ( Msg{Msg::Type::ACK_PART,from, my_id, {}});
      //do not do this: 
      //waiting_for.erase(from);
      //set_edge_status(from,DELETED);
      //(breaks the contract of IN_PART messages, because now we don't need a
      //response to the one we must have sent to them.
      return 1;
    } else {
      buf->push_back ( Msg{Msg::Type::NACK_PART,from, my_id, {}});
      return 1;
    } 
  } else {
    //They're ahead of us, let's wait until we catch up to see what's going on
    this->respond_later.push_back(Msg{Msg::Type::IN_PART,my_id,from,data});
    return 0;
  }
}

size_t GhsState::process_ack_part(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  //we now know that the sender is in our partition. Mark their edge as deleted
  if (waiting_for.find(from)==waiting_for.end()){
    throw std::invalid_argument("We got a IN_PART message from "+std::to_string(from)+" but we weren't waiting for one");
  }
  //@throws:
  set_edge_status(from, DELETED);
  //@throws:
  waiting_for.erase(from);
  return check_search_status(buf);
}

size_t GhsState::process_nack_part(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  //we now know that the sender is in our partition. Mark their edge as deleted
  if (waiting_for.find(from)==waiting_for.end()){
    throw std::invalid_argument("We got a IN_PART message from "+std::to_string(from)+" but we weren't waiting for one");
  }
  auto their_edge = get_edge(from);
  if (!their_edge){
    throw std::invalid_argument("We got a message from "+std::to_string(from)+" but we don't have an edge to them");
  }
  
  if (best_edge.metric_val > their_edge->metric_val){
    best_edge = *their_edge;
  }

  //@throws:
  waiting_for.erase(from);
  return check_search_status(buf);
}

size_t GhsState::check_search_status(std::deque<Msg>* buf){
  
  if (waiting_count() == 0)
  {

    auto e              = mwoe();
    bool am_leader      = (my_part.leader == my_id);
    bool found_new_edge = (e.metric_val < ghs_worst_possible_edge().metric_val);
    bool its_my_edge    = (mwoe().root == my_id);

    if (!am_leader){
      //pass on results, no matter how bad
      return mst_convergecast( Msg::Type::SRCH_RET, {e.peer, e.root, e.metric_val} , buf);
    }

    if (am_leader && found_new_edge && its_my_edge){
      //just add the edge and ping them to join us 
      //(join_us will trigger the broadcast of the new state)
      this->set_edge_status(e.peer, MST);//@throws
      buf->push_back(Msg{Msg::Type::JOIN_US, e.peer, my_id, {e.peer, e.root}});
      return 1;
    }

    if (am_leader && !found_new_edge ){
      //I'm leader, no new edge, let's move on b/c we're done here
      return mst_broadcast( Msg::Type::NOOP, {}, buf);
    }

    if (am_leader && found_new_edge && !its_my_edge){
      //inform the crew to add the edge
      return mst_broadcast( Msg::Type::JOIN_US, {e.peer, e.root}, buf);
    }
  } 

  return 0;
}


size_t GhsState::check_new_level( std::deque<Msg>* buf){

  size_t ret=0;
  auto my_part_level = my_part.level;

  auto msg_itr = respond_later.begin();

  while(msg_itr != respond_later.end() ){
    auto their_level = msg_itr->data[1];
    if (their_level <= my_part_level){
      //ok to answer, they were waiting for us to catch up
      ret+=process_in_part(msg_itr->from, msg_itr->data, buf);
      //req c++11:
      msg_itr = respond_later.erase(msg_itr);
    } else {
      msg_itr ++ ;
    }
  }

  return ret;
}


size_t GhsState::process_join_us(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  // JOIN is tricky. We know (see check_new_level) that we received responses from other partitions, and at the time each partition thought it was not behind our algorithm state. 
  //
  // However, there are a bunch of edge cases that need to be considered. I'm doing by mest to paraphrase the Q/A, but see Lynch 15.5 (pg 517-ish in my edition. 
  //
  // Q: What if we request that another partition joins us, and they are adding a different edge to yet another parition? 
  // - A req B join, but B req C join (which implies C req B)
  // A: That's OK, join, restructure MST, and let the leader-choice uniquely determine next leader for all three
  // - MST goes A->B in terms of distance from leader, then after B+C, A->B->C. The key is C must make a req, and therefore it must be C->B, which triggers B to contain the leader (see next few cases)
  // - OR, MST goes B->C, then A->B, but note that means a new_sheriff msg goes B->A, setting B to contain leader as well.
  //
  // Q: But, how would we know to wait for the *second* MST restructuring after that?
  // - A joins B, restructures, and B joins C and broadcasts a second restructure and it might be that A has already start_round()'d? 
  // A: A join/new-sheriff should over-ride a start_round() or search in progress, obviously, see below  for "receive join".
  //
  // Q: What if a join is received from a node that we're waiting on a NACK/ACK_PART msg for?
  // A: This is especially important to support Merge() operations. When A joins B, with level(A)< level(B), B is stuck waiting on A's response, but A recognizes MWOE leads to B, so it restructures itself *then* responds with ACK_PART.
  
  //
  // Q: But, what if three partitions add eachother in a cycle? 
  // A: Cannot happen -- A<->B<->C<->A implies that A<->C and A<->B is MWOE for A, similarly for B with A,C, etc
  //
  // Q: What if we send a JOIN, but that partition isn't ready to join / still searching?
  // A: They will handle using opposite of next case
  //
  // Q: What if we receive a JOIN but are in the middle of a search?  
  // - we detect this by check waiting_count(), if >0, we're still searching. 
  // - we detect this by check leader of incoming partition even if waiting_count==0. It might be another parition, not us. 
  // A: ??
  //
  //join_us has an edge and a partition as payload
  //this operates on edges, really.  
  //
  //- if neither the root or the peer is us, 
  //  - assert it was received on an MST link
  //  - pass the message on (mst_bc)
  //
  //- if the root of the passed edge is us, 
  //  - assert it was received on an MST link
  //  - we add edge as MST 
  //  - we pass to peer 
  //
  //- if the peer of the passed edge is us
  //  - assert not received on MST link
  //  - assert not same partition
  //  - we add as MST link
  //  - if our level > theirs, 
  //    - ABSORB
  //    - send new sheriff to them (our partition/leader)
  //  - if our level == theirs.
  //    - MERGE
  //    - send new sheriff to them (our partition+1, max(us,them) as leader)
  //    - send new sheriff mst_cc  (our partition+1, max(us,them) as leader)
  //  - if our level < theirs
  //    - ABSORB
  //    - send new sheriff mst_cc(their partition, their leader)
  //
  // 
  return 0;
}

size_t GhsState::process_election(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  return 0;
}

size_t GhsState::process_new_sheriff(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  //This is purely an edge reorganization command
  //If new_sheriff level > ours
  //- The sender is now our parent
  //- adopt partition:
  //- adopt the level
  //- adtop the new laeder
  //- send same message to former parent
  //If not, drop it. It's old news and sender must have received our new-sheriff msg out of order
  return 0;
}

size_t GhsState::process_not_it(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  return 0;
}

size_t GhsState::typecast(const EdgeStatus& status, const Msg::Type &m, const std::vector<size_t> data, std::deque<Msg> *buf)const noexcept{
  size_t sent(0);
  for (const auto & e : outgoing_edges){
    assert(e.root==my_id && "Had an edge in outgoing_edges that was not rooted on me!");
    if ( e.status == status ){
      sent++;
      buf->push_back( Msg{m, e.peer, my_id, data} );
    }
  }
  return sent;
}


size_t GhsState::mst_broadcast(const Msg::Type &m, const std::vector<size_t> data, std::deque<Msg> *buf)const noexcept{
  size_t sent =0;
  for (const auto & e : outgoing_edges){
    assert(e.root==my_id && "Had an edge in outgoing_edges that was not rooted on me!");
    if (e.status == MST && e.peer != parent){
      sent++;
      buf->push_back( Msg{m, e.peer, my_id, data} );
    }
  }
  return sent;
}

size_t GhsState::mst_convergecast(const Msg::Type &m, const std::vector<size_t> data, std::deque<Msg> *buf)const noexcept{
  size_t sent(0);
  for (const auto & e : outgoing_edges){
    assert(e.root==my_id && "Had an edge in outgoing_edges that was not rooted on me!");
    if (e.status == MST && e.peer == parent){
      sent++;
      buf->push_back( Msg{m, e.peer, my_id, data} );
    }
  }
  return sent;
}


Edge ghs_worst_possible_edge(){
  Edge ret;
  ret.root=-1;
  ret.peer=-1;
  ret.status=UNKNOWN;
  ret.metric_val=std::numeric_limits<size_t>::max();
  return ret;
}
