#include "ghs.hpp"
#include "msg.hpp"
#include <algorithm>
#include <assert.h>
#include <deque>
#include <stdexcept>
#include <string>
#include <optional> //req c++20

GhsState::GhsState(AgentID my_id,  std::vector<Edge> edges) noexcept{
  this->my_id =  my_id;
  reset();
  for (const auto e: edges){
    this->set_edge(e);
  }
}

/**
 * Reset the algorithm status completely
 */
bool GhsState::reset() noexcept{
  this->my_part         =  Partition{my_id,0};
  this->parent          =  my_id; //I live alone
  this->waiting_for     =  {};
  this->respond_later   =  {};
  this->best_edge       =  ghs_worst_possible_edge();
  this->best_partition  =  my_part;

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
size_t GhsState::start_round(std::deque<Msg> *outgoing_buffer) noexcept{
  //If I'm leader, then I need to start the process. Otherwise wait
  if (my_part.leader == my_id){
    //nobody tells us what to do but ourselves
    return process_srch(my_id, {}, outgoing_buffer);
  }
  return 0;
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
    case    (Msg::Type::JOIN_OK):{      return  process_join_ok(      msg.from, msg.data, outgoing_buffer);  }
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
      buf->push_back ( Msg{Msg::Type::ACK_PART,from, my_id, {my_part.leader, my_part.level}});
      //do not do this: 
      //waiting_for.erase(from);
      //set_edge_status(from,DELETED);
      //(breaks the contract of IN_PART messages, because now we don't need a
      //response to the one we must have sent to them.
      return 1;
    } else {
      buf->push_back ( Msg{Msg::Type::NACK_PART,from, my_id, {my_part.leader, my_part.level}});
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
  assert(data.size()==2);
  Partition their_part = {data[0],data[1]};

  if (!their_edge){
    throw std::invalid_argument("We got a message from "+std::to_string(from)+" but we don't have an edge to them");
  }
  
  if (best_edge.metric_val > their_edge->metric_val){
    best_edge = *their_edge;
    best_partition = their_part;
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
      //just start the process to join up, rather than sending messages
      assert(e.peer != e.root); //ask me why I check

      return process_join_us(my_id, {e.peer, e.root, get_partition().leader, get_partition().level}, buf);
    }

    if (am_leader && !found_new_edge ){
      //I'm leader, no new edge, let's move on b/c we're done here
      return mst_broadcast( Msg::Type::NOOP, {}, buf);
    }

    if (am_leader && found_new_edge && !its_my_edge){
      //inform the crew to add the edge
      return mst_broadcast( Msg::Type::JOIN_US, {e.peer, e.root, get_partition().leader, get_partition().level}, buf);
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


  // JOIN is tricky. 
  //
  // There's really two kinds: Absorb (joiner loses leader, maybe increases its level), Merge (joiner and joinee both form a new leader together).
  //
  // In general, 
  //
  // - Multiple joins can be processed in any sequence and produce the same result
  // - Merge joins (which increment level) must occur *only* at an edge connecting two partitions, A and B, where MWOE(A) == MWOE(B). 
  // - Merge join should change the algorithm level for at least one of the partitions joining.
  // - [deviation from text] Merge joins are initiated by two absorb actions across the same edge
  // - The key is that if two partitions join eachother, the edge between them is unique and is now a "Core" edge. The leader is always on the new core edge. 
  //
  // Q: What if we request that another partition joins us, and they are adding a different edge to yet another parition? 
  // - A req B join, but B req C join (which implies C req B)
  // A: It should play out like this: 
  // - First, absorb A into B then either:
  //    1. B absorbs into C, taking A with it; followed by C attempting to absorb into B. This kicks off a merge (A+B,C), and the leader is in B or C as required.
  //    2. OR, C absorbs into B, Then A absorbs into B, then B attempts to absorb into C, resulting in a Merge(A+B,C)
  //    3. OR, B and C absorb then merge, followed by A absorbing into B+C.
  // - Either way, one merge happened (level++), and one absorb. 
  //
  // Q: What if a join is received (by node B) from a node (A) that has not sent a  NACK/ACK_PART msg yet (to B)?
  // - This happens when that node (A) makes a decisions about its partitions' MWOE before responding to queries from outside the partition  (e.g., from B)
  // A: Treat it as absorb A into B, (which means A's leader becomes B's leader), and then have B re-start the MWOE search in A's subtree. 
  //
  // Q: But, what if three partitions add eachother in a cycle? 
  // A: Cannot happen:  A<->B<->C<->A implies that A<->C and A<->B is MWOE for A, which is impossible by the unique weights for each edge (See @Edge structure for how this would be accomplished in the field). Similarly for B with A,C, etc. There's a discussion of this ("Component subgraph") in Lynch. 
  //
  // Q: What if we send a JOIN, but that partition isn't ready to join / still searching?
  // A: They will handle using opposite of next case
  //
  // Q: What if we receive a JOIN but are in the middle of a search?  
  // A: This is a more general case of the ACK/NACK discussion above. We treat it as an absorb + search.
  // - Suppose A is searching, and B requests A join B. 
  // - Then two cases: 
  //   1. level(A)>=B, in which case B absorb into A + A trigger new MWOE search is OK
  //   2. level(A)<B, in which case B absorb would lower the level. A may already be in B's partition without knowing it yet. 
  //   - I say this cannot happen. Why? Well, A would not reply to B's IN_PART request, because it does not know wheter it is in B's partition. So, B would not have enough information to declare edge (A,B) as a MWOE. 
  //   - Is there a condition that can increase B's level while waiting for search results? No, a MWOE is required to increase level. 
  //   
  //join_us has an edge and a partition as payload
  //this operates on edges, really.  
  //
  //- if neither the root or the peer is us, 
  //  - assert it was received on an MST link
  //  - pass the message on (mst_broadcast)
  //
  //- if the root of the passed edge is us, 
  //  - assert, waiting_count == 0; that's an error condition otherwise
  //  - assert it was received on an MST link
  //  - we add edge as MST 
  //  - we pass to peer, they will send a NEW_SHERIFF message.
  //
  //- if the peer of the passed edge is us
  //  - assert(our level >= theirs), otherwise how did they get a response from our partition?
  //  - if on an MST link, time to MERGE
  //    - set parent as self
  //    - send new_sheriff (max(us, them), level+1) across all MST links (mst_broadcast)
  //  - if not on an MST link, time to ABSORB:
  //    - we add as MST link
  //    - send new sheriff to them (our leader, our level)
  // 
  //


size_t GhsState::process_join_us(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{

  //we preserve the opportunity to trigger our own joins here with from==my_id
  if (! (get_edge(from) || from==my_id) ){
    throw new std::invalid_argument("No edge to "+std::to_string(from)+" found!");
  }
  assert(data.size()==4);

  auto join_peer  = data[0]; // the side of the edge that is in the other partition 
  auto join_root  = data[1]; // the side of the edge that is in the partition initiating join
  auto join_lead  = data[2]; // the leader, as declared during search, of the peer
  auto join_level = data[3]; // the level of the peer as declared during search

  bool not_involved            = (join_root != my_id && join_peer != my_id);
  bool in_initiating_partition = (join_root == my_id);

  if (not_involved){
    if (join_lead != my_part.leader){
      throw std::invalid_argument("We should never receive a JOIN_US with a different leader, unless we are on the partition boundary");
    }
    if (join_level != my_part.level){
      throw std::invalid_argument("We should never receive a JOIN_US with a different level, unless we are on the partition boundary");
    }  
    return mst_broadcast(Msg::Type::JOIN_US, data, buf);
  }

  std::optional<Edge> edge_to_other_part;

  if (in_initiating_partition){
    //leader CAN be different, even though we're initating, IF we're on an MST
    //link to them
    if (join_lead != my_part.leader && get_edge(join_peer)->status != MST){
      throw std::invalid_argument("We should never receive a JOIN_US with a different leader when we initiate");
    }
    if (join_level != my_part.level){
      throw std::invalid_argument("We should never receive a JOIN_US with a different level when we initiate");
    }  
    edge_to_other_part = get_edge(join_peer);
  } else {
    if (join_lead == my_part.leader){
      throw std::invalid_argument("We should never receive a JOIN_US with same leader from a different partition");
    }
    //level can be same, lower (from another partition), but not higher (we shouldn't have replied)
    if (join_level > my_part.level){
      throw std::invalid_argument("We should never receive a JOIN_US with a higher level when we do not initiate (we shouldnt have replied to their IN_PART)");
    }  
    edge_to_other_part = get_edge(join_root);
  }

  if ( edge_to_other_part->status == MST){
    //we already absorbed once, so now we merge()
    //find leader, send new_sheriff. If sheriff == other guy, send just to him
    //(see "surprised sheriff" in process_new_sheriff).
    auto leader_id = std::max(join_peer, join_root);
    parent = leader_id;
    my_part.level++;
    if (leader_id == my_id){
      return mst_broadcast(Msg::Type::NEW_SHERIFF, {my_id, my_part.level}, buf);
    } else {
      //we initated the second absorb, so let the new sheriff know.
      buf->push_back( Msg{ Msg::Type::NEW_SHERIFF, parent, my_id, {parent, my_part.level}});
      return 1;
    } 
  } else if (edge_to_other_part->status == UNKNOWN){
      if (in_initiating_partition ){
        //requeset abosrb to peer's partition just send it, see what they say
        //(see next one) btw, because we were able to find a MWOE, we know that
        //their level >= ours. Otherwise, they would not have responded to our
        //search (see process_in_part)
        set_edge_status(join_peer, MST);
        buf->push_back(Msg{Msg::Type::JOIN_US, join_peer, my_id, data});
        return 1;
      } else {
        //command them to join ours on same level. 
        //NOTE, if we were waiting for them, they would not respond until their
        //level is == ours, so this should never fail:
        assert(!in_initiating_partition);
        if (join_level > my_part.level){ throw std::invalid_argument("We should\
            never receive a JOIN_US with higher level from a different\
            partition -- they should not have heard our IN_PART response yet");
        }  
        set_edge_status(join_root, MST);
        //send a NEW_SHERIFF only "down" this particular subtree, since leader/level for our partition didn't change
        buf->push_back(Msg{Msg::Type::NEW_SHERIFF, join_root, my_id, {my_part.leader, my_part.level}});
        //and send a "DONE" msg *up* our tree.
        return 1 + mst_convergecast(Msg::Type::JOIN_OK,{}, buf);
      }   
    } else {
    assert(false && "unexpected library error: could not absorb / merge in 'join_us' processing b/c of unexpected edge type between partitions");
    }

  assert(false && "unexpected library error: reached end of function somehow ");
  return 0;
}

size_t GhsState::process_join_ok( AgentID from, std::vector<size_t> data, std::deque<Msg>*buf){

  //we got a message that the join was OK. Time to initiate a new round. 
  if (get_partition().leader == my_id){
    //i'm in charge, start new round
    return start_round(buf);
  } else {
    //we're not, just pass it along
    return mst_convergecast(Msg::Type::JOIN_OK, data, buf);
  }
  
}

//pre-condition: msg contains information about a new leader in a new level greater than ours
//post-condition: level increases, either by joining an advanced partition, or by merging
size_t GhsState::process_new_sheriff(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  //we can only receive these over MST links
  if (get_edge(from)->status != MST){
    throw std::invalid_argument("We should never get NEW_SHERIFF msgs over non-MST links. Maybe an error in join_us");
  }
  assert(data.size()==2);
  auto new_leader = data[0];
  auto new_level  = data[1];

  //special case, we're nominated Surprise!
  if (new_leader == my_id){
    //I'm so flattered you chose me.  This can only happen if we just merged()
    //Which can only happen once per partition (all other joins must be
    //absorb(), which makes me their leader, too)
    //
    //That means this is the end of a round, and we should get moving on the new round.
    parent = my_id;
    set_partition({new_leader,new_level});
    check_new_level(buf);
    return mst_broadcast(Msg::Type::NEW_SHERIFF, {my_part.leader, my_part.level}, buf) 
      + start_round(buf);
  }

  if (from!=parent && new_leader != my_part.leader){
    //reorg in process!
    parent = from;
  }

  //regardless of reorg, we are advanced by joining
  assert(new_level >= my_part.level); //<--something wrong if old new_sheriff msgs are propegating, but we can technically reorg without a level increase during absorb()
  //this might be resolved by choosing std::max(new,old), but is that the right thing to do? No, also need to rebroadcast
  my_part = {new_leader, new_level};

  //and must clean up old pending messages
  check_new_level(buf);

  return mst_broadcast(Msg::Type::NEW_SHERIFF, {my_part.leader, my_part.level}, buf);
}

size_t GhsState::process_election(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
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

AgentID GhsState::get_parent_id() const noexcept{
  return parent;
}
