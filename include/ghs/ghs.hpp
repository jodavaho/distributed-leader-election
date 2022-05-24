#ifndef GHS_HPP
#define GHS_HPP

#include "ghs/msg.hpp"
#include "ghs/graph.hpp"
#include "seque/seque.hpp"
#include <array>


enum GhsError{
  GHS_OK = 0,
  GHS_PROCESS_SELFMSG,
  GHS_PROCESS_NOTME,
  GHS_PROCESS_INVALID_TYPE,
  GHS_PROCESS_REQ_MST,
  GHS_SRCH_INAVLID_SENDER,
  GHS_SRCH_STILL_WAITING,
  GHS_ERR_QUEUE_MSGS,
  GHS_PROCESS_NO_EDGE_FOUND,
  GHS_UNEXPECTED_SRCH_RET,
  GHS_ACK_NOT_WAITING,
  GHS_BAD_MSG,
  GHS_JOIN_BAD_LEADER,
  GHS_JOIN_BAD_LEVEL,
  GHS_JOIN_INIT_BAD_LEADER,
  GHS_JOIN_INIT_BAD_LEVEL,
  GHS_JOIN_MY_LEADER,
  GHS_JOIN_UNEXPECTED_REPLY,
  GHS_ERR_IMPL,
  GHS_CAST_INVALID_EDGE,
  GHS_SET_INVALID_EDGE,
  GHS_PARENT_UNRECOGNIZED,
  GHS_PARENT_REQ_MST,
  GHS_NO_SUCH_PEER,
  GHS_TOO_MANY_AGENTS,
};

const char* ghs_strerror( const GhsError &);

#ifndef ghs_perror
#define ghs_perror(x) fprintf(stderr,"[error] %d:%s", x, ghs_strerror(x))
#endif

template <std::size_t NUM_AGENTS, std::size_t MSG_Q_SIZE>
class GhsState
{

  public:

    /**
     * Initializes the state of the GHS algorithm for this particular node.
     *
     * Requires a Node ID (unique among all agents) Accepts a list of known
     * edges at initialization time. For each edge `set_edge` is called, so the
     * operation is identical to adding them after initialization
     *
     */
    GhsState(AgentID my_id);
    ~GhsState(); 

    /**
     * Changes (or adds) an edge to the outgoing edge list.  If the edge you
     * pass in does not exist, then it will be added.  Edges are considered
     * identical only if they are to-and-from the same nodes. 
     *
     */
    GhsError set_edge(const Edge &e);

    //shims:
    GhsError add_edge(const Edge &e){ return set_edge(e);}

    /**
     *
     * Ensures that an edge to the agent is added, with default metric and UNKNOWN status
     *
     */
    GhsError add_edge_to(const AgentID &to);

    /**
     * Returns the Edge, but only if has_edge(to) would return true.
     *
     * Otherwise, will assert
     */
    GhsError get_edge(const AgentID& to, Edge& out) const;

    /**
     * Returns true if any of the following will work:
     *
     * get_edge(to)
     * set_edge_status(to, ... )
     * get_edge_status(to)
     * set_edge_meteric(to, ... )
     * get_edge_metric(to)
     * set_response_required(to, ... )
     * is_response_required(to)
     * set_response_prompt(to, ... ) 
     * get_response_prompt(to)
     *
     * If it returns false, all of them will fail by asserting.
     */
    bool has_edge( const AgentID &to) const;


    /**
     * Changes the edge status of the edge connecting this node to the AgentID given
     *
     * @throws invalid_argument if you attempt to modify an edge that doesn't
     * exist. 
     */
    GhsError set_edge_status(const AgentID &to, const EdgeStatus &status);
    GhsError get_edge_status(const AgentID&to, EdgeStatus& out);

    /**
     * Changes the edge weight of the edge connecting this node to the AgentID given
     * Returns error / asserts if the edge does not exist
     */
    GhsError set_edge_metric(const AgentID &to, const EdgeMetric m);
    GhsError get_edge_metric(const AgentID &to, EdgeMetric& m);

    GhsError set_leader_id(const AgentID &leader);
    GhsError set_level(const Level &level);

    GhsError set_waiting_for(const AgentID &who, const bool waiting_for);
    GhsError is_waiting_for(const AgentID& who, bool & out_waiting_for);

    GhsError set_response_required(const AgentID &who, const bool response_required);
    GhsError is_response_required(const AgentID &who, bool & response_required);

    GhsError set_response_prompt(const AgentID &who, const InPartPayload& m);
    GhsError get_response_prompt(const AgentID &who, InPartPayload &m);

    /**
     * Returns whatever was set (or initialized) as the AgentID
     *
     * Never fails to return
     */
    AgentID get_id() const;

    /**
     * Returns whatever I believe my parent is
     */
    AgentID get_parent_id() const;

    /**
     * Error: if the id is not on an MST link. Do that first.
     */
    GhsError set_parent_id(const AgentID& id);


    /**
     * Returns whatever I believe my leader is
     */
    AgentID get_leader_id() const;

    /**
     * Returns whatever I believe this partition's level is
     */
    Level get_level() const;

    //getters
    size_t waiting_count() const;
    size_t delayed_count() const;

    Edge mwoe() const;

    /**
     * Sends to MST child links only
     * @return number of messages sent, or <0 on error
     */
    GhsError mst_broadcast(const MsgType, const MsgData&, StaticQueue<Msg, MSG_Q_SIZE> &buf, size_t&) const;

    /**
     * Sends to parent MST link only
     * @return number of mssages sent (it had better be 1 or 0 if this is a root), or <0 on error
     */
    GhsError mst_convergecast(const MsgType, const MsgData&, StaticQueue<Msg, MSG_Q_SIZE>&buf, size_t&)const;

    /**
     * Filters edges by `msgtype`, and sends outgoing message along those that match.
     * @return number of messages sent, or <0 on error
     */
    GhsError typecast(const EdgeStatus status, const MsgType, const MsgData&, StaticQueue<Msg, MSG_Q_SIZE> &buf, size_t&) const;

    //stateful algorithm steps, each returns # of msgs sent (>=0), or <0 on error
    GhsError start_round(StaticQueue<Msg, MSG_Q_SIZE> &outgoing_msgs, size_t&);
    GhsError process(const Msg &msg, StaticQueue<Msg, MSG_Q_SIZE> &outgoing_buffer, size_t&);

    //reset the algorithm state
    GhsError reset();

    //Get the algorithm state
    bool is_converged() const;

    /**
     *
     * Returns the number of peers, which is a counter that is incremented
     * every time you add_edge_to(id) (or variant), with a new id. 
     */
    size_t get_n_peers() const { return n_peers; }


  private:

    /* Search stage messages */
    //each of srch, srch_ret, in_part, ack_part, nack_part are deterministic and straightfoward
    GhsError process_srch(        AgentID from, const SrchPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    GhsError process_srch_ret(    AgentID from, const SrchRetPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    GhsError process_in_part(     AgentID from, const InPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    GhsError process_ack_part(    AgentID from, const AckPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    GhsError process_nack_part(   AgentID from, const NackPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    GhsError process_noop( StaticQueue<Msg, MSG_Q_SIZE>&,  size_t&);
    //This does moderate lifting to determine if the search is complete for the
    //current node, and if so, returns the results to our leader
    GhsError check_search_status( StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);

    /* Join / Merge / Absorb stage message */
    //join_us does some heavy lifting to determine how partitions should be restructured and joined
    GhsError process_join_us(     AgentID from, const JoinUsPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    //After a level change, we may have to do some cleanup responses, this will handle that.
    GhsError check_new_level( StaticQueue<Msg, MSG_Q_SIZE>&, size_t& );

    GhsError                 checked_index_of(const AgentID&, size_t& ) const;
    GhsError                 respond_later(const AgentID&, const InPartPayload);
    GhsError                 respond_no_mwoe( StaticQueue<Msg, MSG_Q_SIZE>&, size_t & );
    AgentID                  my_id;
    AgentID                  my_leader;
    AgentID                  parent;
    Edge                     best_edge;
    Level                    my_level;
    bool                     algorithm_converged;

    size_t                                n_peers;
    std::array<AgentID,NUM_AGENTS>        peers;
    std::array<bool,NUM_AGENTS>           waiting_for_response;
    std::array<Edge,NUM_AGENTS>           outgoing_edges;
    std::array<InPartPayload,NUM_AGENTS>  response_prompt;
    std::array<bool,NUM_AGENTS>           response_required;

};

#include "ghs_impl.hpp"

#endif
