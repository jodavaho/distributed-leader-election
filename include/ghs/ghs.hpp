#ifndef GHS_HPP
#define GHS_HPP

#include "ghs/msg.hpp"
#include "ghs/graph.hpp"
#include "seque/seque.hpp"
#include <array>

typedef int GhsError
const GhsError  GHS_OK                 = 0;
const GhsError  GHS_MSG_INVALID_SENDER =-1;
const GhsError  GHS_MSG_INVALID_TYPE   =-2;
const GhsError  GHS_INVALID_STATE      =-3;
const GhsError  GHS_INVALID_INPART_REC =-4;
const GhsError  GHS_INVALID_SRCHR_REC  =-5;
const GhsError  GHS_INAVLID_JOIN_REC   =-6;
const GhsError  GHS_ERR_IMPL           =-7;
const GhsError  GHS_ERR_NO_SUCH_PARENT =-8;
const GhsError  GHS_NO_SUCH_PEER       =-9;

bool GhsOK(const GhsError &e){
  return e>0;
}

void GhsAssert(const GhsError&e){
  assert(e>0);
}

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
    GhsState(AgentID my_id) noexcept;
    ~GhsState(); 

    /**
     * Changes (or adds) an edge to the outgoing edge list.  If the edge you
     * pass in does not exist, then it will be added.  Edges are considered
     * identical only if they are to-and-from the same nodes. 
     *
     * @throws invalid_argument if you attempt to add an edge
     * that is rooted on another node. 
     *
     * This method is basically @deprecated
     *
     * @Return: 0 if edge updated. 1 if it was inserted (it's new), or <0 on error
     */
    int set_edge(const Edge &e);

    //shims:
    void add_edge(const Edge &e){ set_edge(e);}

    /**
     *
     * Ensures that an edge to the agent is added, with default metric and UNKNOWN status
     *
     */
    void add_edge_to(const AgentID &to);

    /**
     * Returns the Edge, but only if has_edge(to) would return true.
     *
     * Otherwise, will assert
     */
    Edge get_edge(const AgentID& to) const;

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
     * If it returns false, all fo them will fail:
     * - For now, with either assert( has_edge(to) ), or std::invalid_argument
     * - Eventually, with only assert( has_edge(to) )
     */
    bool has_edge( const AgentID &to) const;


    /**
     * Changes the edge status of the edge connecting this node to the AgentID given
     *
     * @throws invalid_argument if you attempt to modify an edge that doesn't
     * exist. 
     */
    void set_edge_status(const AgentID &to, const EdgeStatus &status);
    EdgeStatus get_edge_status(const AgentID&to);

    /**
     * Changes the edge weight of the edge connecting this node to the AgentID given
     *
     * @throws invalid_argument if you attempt to modify an edge that doesn't
     * exist. 
     */
    void set_edge_metric(const AgentID &to, EdgeMetric m);
    EdgeMetric get_edge_metric(const AgentID &to);

    void set_leader_id(const AgentID &leader);
    void set_level(const Level &level);

    void set_waiting_for(const AgentID &who, bool waiting_for);
    bool is_waiting_for(const AgentID& who);

    void set_response_required(const AgentID &who, bool response_required);
    bool is_response_required(const AgentID &who);

    void set_response_prompt(const AgentID &who, const InPartPayload& m);
    InPartPayload  get_response_prompt(const AgentID &who);

    /**
     * Returns whatever was set (or initialized) as the AgentID
     *
     * Never fails to return
     */
    AgentID get_id() const noexcept;

    /**
     * Returns whatever I believe my parent is
     */
    AgentID get_parent_id() const noexcept;

    /**
     * Throws if the id is not on an MST link. Do that first.
     */
    void set_parent_id(const AgentID& id);


    /**
     * Returns whatever I believe my leader is
     */
    AgentID get_leader_id() const noexcept;

    /**
     * Returns whatever I believe this partition's level is
     */
    Level get_level() const noexcept;

    //getters
    size_t waiting_count() const noexcept;
    size_t delayed_count() const noexcept;

    Edge mwoe() const noexcept;

    //useful operations
    //TODO: These apis are ugly -- accept a Payload instead of MsgData and type.
    /**
     * Sends to MST child links only
     * @return number of messages sent, or <0 on error
     */
    int mst_broadcast(const MsgType, const MsgData&, StaticQueue<Msg, MSG_Q_SIZE> &buf, size_t&) const noexcept;

    /**
     * Sends to parent MST link only
     * @return number of mssages sent (it had better be 1 or 0 if this is a root), or <0 on error
     */
    int mst_convergecast(const MsgType, const MsgData&, StaticQueue<Msg, MSG_Q_SIZE>&buf, size_t&)const noexcept;

    /**
     * Filters edges by `msgtype`, and sends outgoing message along those that match.
     * @return number of messages sent, or <0 on error
     */
    int typecast(const EdgeStatus status, const MsgType, const MsgData&, StaticQueue<Msg, MSG_Q_SIZE> &buf, size_t&) const noexcept;

    //stateful algorithm steps, each returns # of msgs sent (>=0), or <0 on error
    int start_round(StaticQueue<Msg, MSG_Q_SIZE> &outgoing_msgs, size_t&) noexcept;
    int process(const Msg &msg, StaticQueue<Msg, MSG_Q_SIZE> &outgoing_buffer, size_t&);

    //reset the algorithm state
    bool reset() noexcept;

    //Get the algorithm state
    bool is_converged() const noexcept;

    /**
     *
     * Returns the number of peers, which is a counter that is incremented
     * every time you add_edge_to(id) (or variant), with a new id. 
     */
    size_t get_n_peers()const { return n_peers; }


  private:

    /* Search stage messages */
    //each of srch, srch_ret, in_part, ack_part, nack_part are deterministic and straightfoward
    int process_srch(        AgentID from, const SrchPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    int process_srch_ret(    AgentID from, const SrchRetPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    int process_in_part(     AgentID from, const InPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    int process_ack_part(    AgentID from, const AckPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    int process_nack_part(   AgentID from, const NackPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    int process_noop( StaticQueue<Msg, MSG_Q_SIZE>&,  size_t&);
    //This does moderate lifting to determine if the search is complete for the
    //current node, and if so, returns the results to our leader
    int check_search_status( StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);

    /* Join / Merge / Absorb stage message */
    //join_us does some heavy lifting to determine how partitions should be restructured and joined
    int process_join_us(     AgentID from, const JoinUsPayload&, StaticQueue<Msg, MSG_Q_SIZE>&, size_t&);
    //After a level change, we may have to do some cleanup responses, this will handle that.
    int check_new_level( StaticQueue<Msg, MSG_Q_SIZE>&, size_t& );

    bool                     index_of(const AgentID&, size_t& out_idx) const;
    void                     respond_later(const AgentID&, const InPartPayload &m);
    int                      respond_no_mwoe( StaticQueue<Msg, MSG_Q_SIZE>& );
    AgentID                  my_id;
    AgentID                  my_leader;
    AgentID                  parent;
    Edge                     best_edge;
    Level                    my_level;
    bool                     algorithm_converged;

    std::array<AgentID,NUM_AGENTS>        peers;
    std::size_t                           n_peers;
    std::array<bool,NUM_AGENTS>           waiting_for_response;
    std::array<Edge,NUM_AGENTS>           outgoing_edges;
    std::array<InPartPayload,NUM_AGENTS>  response_prompt;
    std::array<bool,NUM_AGENTS>           response_required;

};

#include "ghs_impl.hpp"

#endif
