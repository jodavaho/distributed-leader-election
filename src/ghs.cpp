#include "ghs.hpp"
#include "msg.hpp"
#include <algorithm>
#include <assert.h>
#include <deque>
#include <stdexcept>
#include <sstream>
#include <string>
#include <optional> //req c++20

GhsState::GhsState(AgentID my_id,  std::vector<Edge> edges) noexcept{
  this->my_id =  my_id;
  reset();
  for (const auto e: edges){
    this->set_edge(e);
  }
}

std::ostream& operator << ( std::ostream& outs, const GhsState & s){
  outs<<"{id:"<<s.get_id()<<" ";
  outs<<"leader:"<<s.get_leader_id()<<" ";
  outs<<"level:"<<s.get_level()<<" ";
  outs<<"waiting:"<<s.waiting_count()<<" ";
  outs<<"delayed:"<<s.delayed_count()<<" ";
  outs<<"converged:"<<s.is_converged()<<" ";
  outs<<"("<<s.dump_edges()<<")";
  outs<<"}";
  return outs;
}

/**
 * Reset the algorithm status completely
 */
bool GhsState::reset() noexcept{
  this->my_part              =  Partition{my_id,0};
  this->parent               =  my_id;                      
  this->waiting_for          =  {};
  this->respond_later        =  {};
  this->best_edge            =  ghs_worst_possible_edge();
  this->algorithm_converged  =  false;

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

  if (e.status != MST && e.peer != my_id ){
    throw std::invalid_argument("Cannot add non MST / non-self edge as parent!");
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
    return process_srch(my_id, {my_part.leader, my_part.level}, outgoing_buffer);
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
    case    (Msg::Type::NOOP):{         return  process_noop( outgoing_buffer ); }
    //case    (Msg::Type::ELECTION):{     return  process_election(     msg.from, msg.data, outgoing_buffer);  }
    //case    (Msg::Type::NOT_IT):{       return  process_not_it(       this,msg.from, msg.data);      }
    default:{ throw std::invalid_argument("Got unrecognzied message"); }
  }
  return true;
}

size_t GhsState::process_srch(  AgentID from, std::vector<size_t> data, std::deque<Msg>*buf)
{
  assert(data.size()==2);
  if (from !=my_id && get_edge(from)->status !=MST){
    throw std::invalid_argument("Unknown sender");
  }
  assert(from == my_id ||  get_edge(from)->status == MST); // now that would be weird if it wasn't

  if (waiting_for.size() != 0 ){
    throw std::invalid_argument(" Waiting but got SRCH ");
  }
  assert(waiting_for.size()==0 && " We got a srch msg while still waiting for results!");

  //grab the new partition information, since only one node / partition sends srch() msgs.
  auto leader = data[0];
  auto level  = data[1];
  my_part = {leader,level};
  //also note our parent may have changed
  parent=from;

  //initialize the best edge to a bad value for comparisons
  best_edge = ghs_worst_possible_edge();
  best_edge.root = my_id;

  //we'll cache outgoing messages temporarily
  std::deque<Msg> srchbuf;

  //first broadcast
  size_t srch_sent = mst_broadcast(Msg::Type::SRCH, {my_part.leader, my_part.level}, &srchbuf);

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

  //make sure to check_new_level, since our level may have changed, above. 
  return srch_sent + part_sent + check_new_level(buf);
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
      //just start the process to join up, rather than sending messages
      assert(e.peer != e.root); //ask me why I check

      return process_join_us(my_id, {e.peer, e.root, get_partition().leader, get_partition().level}, buf);
    }

    if (am_leader && !found_new_edge ){
      //I'm leader, no new edge, let's move on b/c we're done here
      return process_noop( buf );
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


//  If there's a bug, it's probably in here. 
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
      std::cerr<<"My leader: "<<my_part.leader<<std::endl;
      std::cerr<<"join_lead: "<<join_lead<<std::endl;
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
    auto leader_id = std::max(join_peer, join_root);
    my_part.leader = leader_id;
    my_part.level++;
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
  } else if (edge_to_other_part->status == UNKNOWN) {
      if (in_initiating_partition ){
        //In this case, we're sending a JOIN_US without hearing from them yet.
        //We may not be their MWOE, which would make this an "absorb" case.
        //just send it, see what they say (see next one). btw, because we were
        //able to find a MWOE, we know that their level >= ours. Otherwise,
        //they would not have responded to our search (see process_in_part). So
        //this absorb request is valid and setting their link as MST is OK. 
        set_edge_status(join_peer, MST);
        buf->push_back(Msg{Msg::Type::JOIN_US, join_peer, my_id, data});
        return 1;
      } else {

        assert(!in_initiating_partition);

        //In this case, we received a JOIN_US from another partition, one that
        //we have not yet recognized or marked as our own MWOE. This means they
        //are a prime candidate to absorb into our partition. 
        //NOTE, if we were waiting for them, they would not respond until their
        //level is == ours, so this should never fail:
        assert(my_part.level>=join_level);

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

size_t GhsState::process_noop(std::deque<Msg> *buf){
  algorithm_converged=true;
  return mst_broadcast(Msg::Type::NOOP, {},buf);
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

AgentID GhsState::get_leader_id() const noexcept{
  return my_part.leader;
}

AgentID GhsState::get_level() const noexcept{
  return my_part.level;
}

bool GhsState::is_converged() const noexcept{
  return algorithm_converged;
}

std::string  GhsState::dump_edges() const noexcept{
  std::stringstream ss;
  ss<<"( ";
  for (auto e:outgoing_edges){
    ss<<" ";
    ss<<e.root<<"-->"<<e.peer<<" ";
    switch (e.status){
      case UNKNOWN: {ss<<"---";break;}
      case MST: {ss<<"MST";break;}
      case DELETED: {ss<<"DEL";break;}
    }
    if (e.peer == parent){
      ss<<"+P";
    } else {
      ss<<"  ";
    }
    ss<<" "<<e.metric_val<<";";
  }
  ss<<")";
  return ss.str();
}
