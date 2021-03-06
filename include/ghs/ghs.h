/**
 * @copyright
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
 * @file ghs.h
 * @brief **The main GhsState object**
 *
 */

#ifndef GHS_H
#define GHS_H

#include "ghs/msg.h"
#include "ghs/agent.h"
#include "ghs/level.h"
#include "ghs/edge.h"
#include "le/errno.h"
#include "seque/static_queue.h"
#include <array>

using seque::StaticQueue;

/**
 * Leader Election
 */
namespace le{

  /**
   * The GHS namespace
   */
  namespace ghs{

    /** 
     * @brief **The main state machine for the GHS algorithm**
     *
     * GhsState is the message-driven state machine that executes the GHS
     * algorithm. It receives incoming messages from  a communication layer,
     * and returns the next batch of messages to send. When completed,
     * is_converged() will return true.
     *
     * You are responsible for describing the communication graph by calling,
     * probably, set_edge() and then starting the algorithm by calling
     * start_round() on at least one node (the root), but most likely just call
     * it on all nodes, which will generate the first set of messages to send.
     *
     * Then, as response messages come in from other nodes, just feed them into
     * process() until is_converged() is true. 
     *
     */ 
    template <std::size_t NUM_AGENTS, std::size_t MSG_Q_SIZE>
      class GhsState
      {

        public:

          /**
           * Initializes the state of the GHS algorithm for this particular node.
           *
           * Requires a agent_t to represent the node id, which will be used in
           * all incoming and outgoing mesasages (unique among all agents).
           *
           * Requires a list of le::ghs::Edge structures that represent the
           * communication links to other agents that will not be modified
           * during execution.
           *
           * The edge list may contain any number of edges (up to NUM_AGENTS). This class will ignore (not copy in) any edge that:
           *
           * - Is not rooted on this node (Edge.root != my_id)
           * - Is directed to this node (Edge.peer == my_id)
           * - Has either peer or root set to le::ghs::NO_AGENT
           * - Has metric_val set to that of worst_edge()
           * - otherwise does not pass is_valid()
           *
           * The following conditions will produce undefined behavior:
           *
           * - Passing in two or more edges with the same peer and root
           *
           * After the construction, you can verify the number of copied edges with get_n_peers(). 
           *
           * @param my_id of type agent_t that tells the class which edges to consider
           * @param edges a set of le::ghs::Edge structures, which are filtered and stored internally to determine message destinations.
           * @param num_edges the length of the edge set
           *
           */
          GhsState(agent_t my_id, Edge* edges, size_t num_edges);
          ~GhsState(); 

          /**
           * **ONLY IF** this node is the root of an MST (even an MST with only itself
           * as a member) **THEN** this function will enqueue the first set of
           * messages to send to all peers, and set up the internal state of
           * the algorithm to be ready to process the responses. 
           *
           * In short it:
           *   * checks to make sure we're not already in a search phase, exiting with error if we are.
           *   * resets the MWOE to a default value
           *   * creates a msg::SrchPayload and calls mst_broadcast()
           *
           * Calling start_round() while in the middle of a round will
           * essentially lose all state, such that incomign messages that are
           * not a response to *these outgoing messages* will likely cause
           * errors.
           *
           * However, no edge statuses are changed, so executing start_round is
           * safe if you already know of some MST links and have edited them
           * in, or have somehow terminated a round and want to resume it. 
           *
           * @param StaticQeueue in which to enque outgoing messages
           * @param size_t the number of messages enque'd
           * @return le::Errno OK if successful
           * @return le::Errno SRCH_STILL_WAITING if waiting_count() is not zero
           */
          le::Errno start_round(StaticQueue<Msg, MSG_Q_SIZE> &outgoing_msgs, size_t&);

          /**
           * The main class entry point. It will puplate the outgoing_buffer
           * with message that should be sent as a response to the passed-in message.
           * You can execute the entire algorithm simply by calling process()
           * with a msg::SrchPayload message properly constructed (but use
           * start_round() for this), then feeding in all the response
           * messages. 
           *
           * @param Msg to process
           * @param StaticQueue into which to push the response messages
           * @param sz the size_t that will be set to the number of messages added to outgoing_buffer on success, or left unset otherwise
           * @see Msg
           * @see le::Errno
           */
          le::Errno process(const Msg &msg, StaticQueue<Msg, MSG_Q_SIZE> &outgoing_buffer, size_t& sz);

          /**
           * @return true if the state machine believes that a global MST has converged
           * @return false otherwise
           */
          bool is_converged() const;

          /**
           * Returns the number of peers, which is a counter that is incremented
           * every time you add_edge_to(id) (or variant), with a new id. 
           */
          size_t get_n_peers() const { return n_peers; }



          /**
           * Populates the given edge with any stored edge that connects this
           * agent to another agent. If we are unaware of that agent or do not
           * have an edge, return error code. 
           *
           * @param to an agent_t to look up
           * @param out and Edge to populate as an out parameter
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           * @return le::Errno NO_SUCH_PEER if edge cannot be found
           * @return le::Errno OK if successful
           * @see has_edge()
           */
          le::Errno get_edge(const agent_t& to, Edge& out) const;

          /**
           * Returns true if any of the following will work:
           *
           * ```
           * get_edge()
           * set_edge_status()
           * get_edge_status()
           * set_edge_metric()
           * get_edge_metric()
           * set_response_required()
           * is_response_required()
           * set_response_prompt() 
           * get_response_prompt()
           * ```
           *
           * If it returns false, all of them will fail by returning something
           * other than le::Errno OK. 
           *
           */
          bool has_edge( const agent_t to) const;

          /**
           *
           * Returns the edge status to the given agent. 
           *
           * @param to agent_t identifier
           * @param out the status_t that is populated if the function is successful
           * @return OK if successful
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           */
          le::Errno get_edge_status(const agent_t&to, status_t& out) const;


          /**
           *
           * Returns the edge metric to the given agent. 
           *
           * @param to agent_t identifier
           * @param m the metric_t that is populated if the function is successful
           * @return OK if successful
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           */
          le::Errno get_edge_metric(const agent_t &to, metric_t& m) const;


          /** 
           *
           * returns the waiting status for the given agent. 
           *
           * @param agent_t who 
           * @param bool waiting for response (true) or not waiting for response (false)
           * @return OK if successful and `waiting_for` is a valid return
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id and `waiting_for` may have any value
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id and `waiting_for` may have any value
           */
          le::Errno is_waiting_for(const agent_t& who, bool & out_waiting_for);

          /** 
           *
           * returns the response-delayed status for the given agent. 
           *
           * @param agent_t who 
           * @param bool waiting to send (true) or not waiting (false)
           * @return OK if successful and `waiting_for` is a valid return
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id and `waiting_for` may have any value
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id and `waiting_for` may have any value
           */
          le::Errno is_response_required(const agent_t &who, bool & response_required);

          /**
           * Returns the message that triggered a delay in response.
           *
           * @param agent_t who sent the message
           * @param InPartPayload the outgoing payload of the message that we cannot respond to yet
           * @return le::Errno OK if successful
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           */
          le::Errno get_response_prompt(const agent_t &who, msg::InPartPayload &m);

          /**
           * Returns whatever was set (or initialized) as the agent_t for this state machine
           *
           * Never fails to return
           *
           * @return agent_t for this class's id.  
           */
          agent_t get_id() const;

          /**
           * Returns whatever I believe my parent is
           * @return agent_t corresponding to the parent id. Could be self!
           */
          agent_t get_parent_id() const;


          /**
           * Returns whatever I believe my leader is
           */
          agent_t get_leader_id() const;

          /**
           * Returns whatever I believe this partition's level is
           */
          level_t get_level() const;

          /**
           * Returns the number of agents to which we have already sent IN_PART
           * messages, but from which we have not yet received ACK_PART or
           * NACK_PART messages.
           *
           * @return size_t 
           * @see set_waiting_for()
           * @see msg::InPartPayload
           * @see Msg
           */
          size_t waiting_count() const;

          /**
           * Returns the number of agents from which we have received an
           * IN_PART message that we have not responded to. 
           *
           * @return size_t 
           * @see set_response_required()
           * @see msg::InPartPayload
           * @see Msg
           */
          size_t delayed_count() const;

          /**
           * Returns the current minimum weight outgoing edge (MWOE).
           *
           * This is the edge we would add to our partition if you forced us to
           * chose from our minimum spanning tree rooted at ourself. To find
           * the global MWOE, these are passed UP the MST using
           * mst_convergecast(), with a msg::SrchRetPayload. At each
           * node, the msg::SrchRetPayload is compared to our mwoe() to determine
           * the actual best edge all the way up to the root of the MST for
           * this partition. After that, a msg::JoinUsPayload is sent back from the
           * root using mst_broadcast() to trigger the process of adding that
           * edge to the MST
           *
           * @return size_t 
           * @see set_response_required()
           * @see msg::InPartPayload
           * @see Msg
           */
          Edge mwoe() const;

          
          /** 
           * A much-called function that returns the index of the given agent.
           * The index corresponds to a number 0 to N-1 for N agents, such that
           * all data about that agent can be stored in consecutive memory.
           * This is not a hash function! It simply searches as an O(n)
           * operation, the memory for the matching ID.
           *
           * @return le::Errno OK if the index was found
           * @return le::Errno NO_SUCH_PEER if not
           * @return le::Errno IMPL_REQ_PEER_MY_ID if you requsted index to this agent
           */
          le::Errno                 checked_index_of(const agent_t&, size_t& ) const;



          /**
           * Sends messages to MST child links only. There are very good
           * reasons for using MST links even for non-ghs messages, so this is
           * public. 
           *
           * For example, this ensures each node only receives one copy, even
           * if it is a "bottleneck" leading towards many agents.
           *
           * Functionally equivalent to:
           *
           * ```
           * Mst m;
           * StaticQueue buf;
           * size_t qsz;
           * return mst_typecast(MST, m.type, m.data, buf, qsz);
           * ```
           *
           * @param msg::Type denoting what type of message to send
           * @param msg::Data denoting what message data to broadcast
           * @param StaticQueue in which to queue the outgoing messages
           * @param size_t denoting how many messages were enqueued *only* if OK is returned.
           * @return le::Errno OK if everything went well
           * @return CAST_INVALID_EDGE if we found an edge without us as root 
           * @see set_edge_status()
           * @see mst_typecast()
           * @see mst_convergecast()
           */
          le::Errno mst_broadcast(const msg::Type, const msg::Data&, StaticQueue<Msg, MSG_Q_SIZE> &buf, size_t&) const;


          /**
           * The opposite of mst_broadcast, will send messages "UP" the MST to the root.
           *
           * useful for conducting "reduce" operations on an MST, assuming it
           * is combined with a useful data reduction strategy. 
           *
           * In GHS, the reduction strategy is to compare msg::SrchRetPayload from
           * all incoming MST links, and pass the minimum weight edge up to the
           * parent. 
           *
           * Is actually implemented with a search across all edges for one
           * of type MST and with peer matching our parent id. 
           *
           * @param msg::Type denoting what type of message to send
           * @param msg::Data denoting what message data to broadcast
           * @param StaticQueue in which to queue the outgoing messages
           * @param size_t denoting how many messages were enqueued *only* if OK is returned.
           * @return le::Errno OK if everything went well
           * @return CAST_INVALID_EDGE if we found an edge without us as root 
           * @see set_edge_status()
           * @see mst_typecast()
           * @see mst_convergecast()
           */
          le::Errno mst_convergecast(const msg::Type, const msg::Data&, StaticQueue<Msg, MSG_Q_SIZE>&buf, size_t&)const;

          /**
           * Filters edges by `msgtype`, and sends outgoing message along those
           * that match. 
           *
           * @param status_t the edge status along which to send messages. 
           * @param msg::Type denoting what type of message to send
           * @param msg::Data denoting what message data to broadcast
           * @param StaticQueue in which to queue the outgoing messages
           * @param size_t denoting how many messages were enqueued *only* if OK is returned.
           * @return le::Errno OK if everything went well
           * @return CAST_INVALID_EDGE if we found an edge without us as root 
           * @see set_edge_status()
           * @see mst_typecast()
           * @see mst_convergecast()
           */
          le::Errno typecast(const status_t status, const msg::Type, const msg::Data&, StaticQueue<Msg, MSG_Q_SIZE> &buf, size_t&) const;


        private:


          /**
           * Changes (or adds) an edge to the outgoing edge list.  If the edge
           * you pass in does not exist, then it will be added.  Edges are
           * considered identical if and only if they are to-and-from the same
           * nodes. 
           *
           * The edge must satisfy:
           *   * To someone else (peer) 
           *   * From us (root)
           *   * Not weight 0 or otherwise same weight as worst_edge()
           *
           *  @return le::Errno OK if successful
           *  @return le::Errno SET_INVALID_EDGE if edge has root!=my_id
           *  @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           *  @return le::Errno TOO_MANY_AGENTS if this is a new edge and we would exceed MAX_AGENTS
           *  @param e an Edge to add
           *  @see Edge
           *  @see le::Errno 
           */
          le::Errno set_edge(const Edge &e);

          /**
           * Does nothing more than call set_edge(e)
           * @see set_edge()
           */
          le::Errno add_edge(const Edge &e){ return set_edge(e);}

          /**
           * Changes the internally stored Edge to have a status_t matching `status`.
           *
           * Functionally equivalent to:
           *
           * ```
           * status_t desired;
           * Edge e;
           * agent_t to;
           * get_edge(to,e);
           * e.status = desired;
           * set_edge(to,e);
           * ```
           *
           * **Warning** if you really need to remove an edge from the MST
           * construction, perhaps because it is temporarily unavailable, you
           * might be tempted to set the status to DELETED. I would recommend
           * you not do this unless waiting_count() and delayd_count() is zero,
           * and you are confident that you will not soon receive msg::SrchPayload
           * messages from other nodes over that link.  
           *
           * @param to agent_t identifier
           * @param status the status_t to set
           * @return OK if successful
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           * @see has_edge()
           */
          le::Errno set_edge_status(const agent_t &to, const status_t &status);
          
          /**
           * Changes the internally stored Edge to have a metric_t matching `m`.
           *
           * Functionally equivalent to:
           *
           * ```
           * metric_t desired;
           * Edge e;
           * agent_t to;
           * get_edge(to,e);
           * e.metric_val = desired;
           * set_edge(to,e);
           * ```
           * @param to agent_t identifier
           * @param m the metric_t to set
           * @return OK if successful
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           * @see has_edge()
           */
          le::Errno set_edge_metric(const agent_t &to, const metric_t m);

          /**
           * Sets the leader of this node to the given agent_t
           * @return le::Errno OK. Never fails
           */
          le::Errno set_leader_id(const agent_t &leader);
  
          /**
           * Sets the level of this node to the given level_t
           * @return le::Errno OK. Never fails
           */
          le::Errno set_level(const level_t &level);

          /** 
           *
           * Sets the flag that denotes we have sent an IN_PART message to
           * this agent, but have not yet received a response (true) or have
           * received their response (false).  
           *
           * @param agent_t who 
           * @param bool waiting for (true) or not waiting for (false)
           * @return OK if successful
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           */
          le::Errno set_waiting_for(const agent_t &who, const bool waiting_for);
          
          /** 
           *
           * Sets the flag that denotes we have received an IN_PART message,
           * but are not yet ready to respond. This occurs when the senders
           * level is higher than ours, because we may just not yet know that
           * we are actually part of their partition. We will know for sure
           * when our level is == theirs, and we know the other agent will not
           * respond if their level < ours. 
           *
           * If you wish to "manually steer" the ghs algorithm using this
           * function, then you should also use set_response_prompt()
           *
           * @param agent_t who 
           * @param bool waiting to send (true) or not waiting (false)
           * @return OK if successful
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           * @see respond_later()
           * @see process_in_part()
           */
          le::Errno set_response_required(const agent_t &who, const bool response_required);

          /**
           * Caches the message that triggered a delay in response, so that we
           * can look it up later to check if our level matches the requester's
           * level. We do that check whenever our level changes. 
           *
           * @param agent_t who sent the message
           * @param msg::InPartPayload the payload of the message that we cannot respond to yet
           * @return le::Errno OK if successful
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           */
          le::Errno set_response_prompt(const agent_t &who, const msg::InPartPayload& m);

          /** 
           * Sets the MST parent link (of which we have only one!). The edge
           * to the parent must satisfy one of:
           *   * get_edge_status(id,s) returns an MST edge
           *   * agent_t == get_id()
           *
           * @return le::Errno OK if successful
           * @return le::Errno NO_SUCH_PEER if we cannot find the given agent id
           * @return le::Errno IMPL_REQ_PEER_MY_ID if edge has peer==my_id
           * @return le::Errno PARENT_UNRECOGNIZED if `!has_edge(id)`
           * @return le::Errno PARENT_REQ_MST if we do not have an MST link to that `id`
           * @see set_edge_status()
           * @see Edge
           */
          le::Errno set_parent_id(const agent_t& id);


          /**
           * Reset the algorithm state, as though this object were just constructed (but preserving my_id)
           */
          le::Errno reset();



          /**
           * Called by process() with specifically msg::SrchPayload messages
           */
          le::Errno process_srch(        agent_t from, const msg::SrchPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
          /**
           * Called by process() with specifically msg::SrchRetPayload messages
           */
          le::Errno process_srch_ret(    agent_t from, const msg::SrchRetPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
          /**
           * Called by process() with specifically msg::InPartPayload messages
           */
          le::Errno process_in_part(     agent_t from, const msg::InPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
          /**
           * Called by process() with specifically msg::AckPartPayload messages
           */
          le::Errno process_ack_part(    agent_t from, const msg::AckPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
          /**
           * Called by process() with specifically msg::NackPartPayload messages
           */
          le::Errno process_nack_part(   agent_t from, const msg::NackPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
          /**
           * Called by process() with specifically msg::NoopPayload messages
           */
          le::Errno process_noop( StaticQueue<Msg, MSG_Q_SIZE>&,  size_t&);
          
          /**
           * This does moderate lifting to determine if the search is complete for the
           * current node, and if so, returns the results to our leader
           */
          le::Errno check_search_status( StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);

          /* Join / Merge / Absorb stage message */
          //join_us does some heavy lifting to determine how partitions should be restructured and joined
          le::Errno process_join_us(     agent_t from, const msg::JoinUsPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);

          /**
           * After our level changes, we may have to do some cleanup, including responding to old messages, so this function completes that check and buffers the new messages if required
           */
          le::Errno check_new_level( StaticQueue<Msg, MSG_Q_SIZE>&, size_t& );

          /**
           * This will store the message and set a flag that we should respond to this later.
           * @see check_new_level
           */
          le::Errno                  respond_later(const agent_t&, const msg::InPartPayload);
          
          /**
           * This will respond that a search for an outgoing MWOE was inconclusive. This usually means the round is about to end and the MST construction is completed
           */
          le::Errno                  respond_no_mwoe( StaticQueue<Msg, MSG_Q_SIZE>&, size_t & );


          agent_t                  my_id;
          agent_t                  my_leader;
          level_t                  my_level;
          bool                     algorithm_converged;
          
          Edge                     best_edge;

          size_t                                n_peers;
          std::array<agent_t,NUM_AGENTS>        peers;
          std::array<bool,NUM_AGENTS>           waiting_for_response;
          std::array<Edge,NUM_AGENTS>           outgoing_edges;
          std::array<msg::InPartPayload,NUM_AGENTS>  response_prompt;
          std::array<bool,NUM_AGENTS>           response_required;

      };

#include "ghs_impl.hpp"

  }
}

#endif
