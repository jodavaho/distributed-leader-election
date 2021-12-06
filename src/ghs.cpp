#include "ghs.hpp"
#include "msg.hpp"
#include <assert.h>
#include <deque>
#include <stdexcept>
#include <algorithm>

GHS_STATUS  GHS_OK(0);
GHS_STATUS  GHS_ERR(1);

GHS_State::GHS_State(AgentID my_id, int num_agents, std::vector<Edge> edges) noexcept{
  this->num_agents     =  num_agents;
  this->my_id          =  my_id;
  this->my_part        =  Partition{my_id,0};
  this->parent         =  -1; //I live alone
  this->waiting_for    =  {};
  this->best_edge      =  ghs_worst_possible_edge();

  this->outgoing_edges.reserve(num_agents);

  for (int i=0;i<num_agents;i++){
    outgoing_edges[i]=Edge{my_id, i, UNKNOWN, 0};
  }

  for (const auto e: edges){
    this->set_edge(e);
  }
}

/**
 * Reset the algorithm status completely
 */
bool GHS_State::reset() noexcept{
  waiting_for.clear();
  this->parent = -1;
  this->my_part = Partition{my_id,0};
  for (size_t i=0;i<outgoing_edges.size();i++){
    outgoing_edges[i].status=UNKNOWN;
  }
  return true;
}


/** 
 *
 * You must set an edge as MST using set_edge, BEFORE you call this method to
 * set that MST edge as a link to parent. 
 *
 * That MST edge's peer is considered parent from now on
 *
 * @throws invalid_argument if edge is not MST or if the edge does not exist in
 * our outgoing edges
 *
 */
void GHS_State::set_parent_edge(const Edge&e) {

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

/**
 * Add or update edge. Searches by e.peer.
 *
 * @Return: 0 if edge updated. 1 if it was inserted (it's new)
 */
int GHS_State::set_edge(const Edge &e) noexcept{

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
  assert(outgoing_edges.size() <= (size_t) num_agents-1 && "Inserted edge created more outgoing agents than expected, given num_agents");
  return 1;
}

/**
 * Set the partition for this guy/gal
 */
void GHS_State::set_partition(const Partition &p) noexcept{
  my_part = p;
}

/**
 * Queue up the start of the round
 *
 */
void GHS_State::start_round(std::deque<Msg> *outgoing_buffer) noexcept{
  //If I'm leader, then I need to start the process. Otherwise wait
  if (my_part.leader == my_id){
    //nobody tells us what to do but ourselves
    process_srch(my_id, {}, outgoing_buffer);
  }
}

size_t GHS_State::waiting_count() const noexcept
{
  return waiting_for.size();
}

Edge GHS_State::mwoe() const noexcept{
  return best_edge;
}

int GHS_State::process(const Msg &msg, std::deque<Msg> *outgoing_buffer){
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

int GHS_State::process_srch(  AgentID from, std::vector<int> data, std::deque<Msg>*buf)
{
  if (from!=my_part.leader && from!=parent){
    //something is tragically wrong here, one of thes must be true
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

int GHS_State::process_srch_ret(  AgentID from, std::vector<int> data, std::deque<Msg>*buf)
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
  theirs.root        =  data[0];
  theirs.peer        =  data[1];
  theirs.status      =  UNKNOWN;
  theirs.metric_val  =  data[2];

  if (theirs.metric_val < best_edge.metric_val){
    best_edge.root        =  theirs.root;
    best_edge.peer        =  theirs.peer;
    best_edge.metric_val  =  theirs.metric_val;
  }

  size_t sent = 0;
  if (waiting_count() == 0){

    //Who's in charge?
    if ( my_part.leader == my_id ){
      //I'm in charge
      if (best_edge.metric_val == ghs_worst_possible_edge().metric_val){
        //no edge found, MST complete, time to pass the baton
        sent += mst_broadcast( Msg::Type::ELECTION, {}, buf);
      } else {
        //add the found edge
        sent += mst_broadcast( Msg::Type::JOIN_US, {best_edge.root, best_edge.peer}, buf);
      }
    } else {
      //I'm not in charge, relay results
      sent += mst_convergecast( Msg::Type::SRCH_RET, {best_edge.root, best_edge.peer, (int) best_edge.status, best_edge.metric_val} , buf);
    }
  }
  return sent;
}

int GHS_State::process_in_part(  AgentID from, std::vector<int> data, std::deque<Msg>*buf)
{

  return 0;
}

int GHS_State::process_ack_part(  AgentID from, std::vector<int> data, std::deque<Msg>*buf)
{
  return 0;
}

int GHS_State::process_nack_part(  AgentID from, std::vector<int> data, std::deque<Msg>*buf)
{
  return 0;
}

int GHS_State::process_join_us(  AgentID from, std::vector<int> data, std::deque<Msg>*buf)
{
  return 0;
}

int GHS_State::process_election(  AgentID from, std::vector<int> data, std::deque<Msg>*buf)
{
  return 0;
}

int GHS_State::process_new_sheriff(  AgentID from, std::vector<int> data, std::deque<Msg>*buf)
{
  return 0;
}

int GHS_State::process_not_it(  AgentID from, std::vector<int> data, std::deque<Msg>*buf)
{
  return 0;
}

size_t GHS_State::typecast(const EdgeStatus& status, const Msg::Type &m, const std::vector<int> data, std::deque<Msg> *buf)const noexcept{
  size_t sent(0);
  for (const auto & e : outgoing_edges){
    assert(e.root==my_id && "Had an edge in outgoing_edges that was not rooted on me!");
    if (e.status == status && e.peer != parent){
      sent++;
      buf->push_back( Msg{m, e.peer, my_id, data} );
    }
  }
  return sent;
}


size_t GHS_State::mst_broadcast(const Msg::Type &m, const std::vector<int> data, std::deque<Msg> *buf)const noexcept{
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

size_t GHS_State::mst_convergecast(const Msg::Type &m, const std::vector<int> data, std::deque<Msg> *buf)const noexcept{
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
  ret.metric_val=std::numeric_limits<int>::max();
  return ret;
}
