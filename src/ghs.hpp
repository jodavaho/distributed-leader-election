#ifndef GHS_HPP
#define GHS_HPP

#include "msg.hpp"
#include "graph.hpp"
#include "seque.hpp"
#include <array>

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
     * @Return: 0 if edge updated. 1 if it was inserted (it's new)
     */
    size_t set_edge(const Edge &e);

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
    void set_edge_metric(const AgentID &to, size_t);
    size_t get_edge_metric(const AgentID &to);

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
     * @return number of messages sent
     */
    size_t mst_broadcast(const MsgType, const MsgData&, StaticQueue<Msg, MSG_Q_SIZE> &buf) const noexcept;

    /**
     * Sends to parent MST link only
     * @return number of mssages sent (it had better be 1 or 0 if this is a root)
     */
    size_t mst_convergecast(const MsgType, const MsgData&, StaticQueue<Msg, MSG_Q_SIZE>&buf)const noexcept;

    /**
     * Filters edges by `msgtype`, and sends outgoing message along those that match.
     * @return number of messages sent
     */
    size_t typecast(const EdgeStatus status, const MsgType, const MsgData&, StaticQueue<Msg, MSG_Q_SIZE> &buf) const noexcept;

    //stateful algorithm steps
    size_t start_round(StaticQueue<Msg, MSG_Q_SIZE> &outgoing_msgs) noexcept;
    size_t process(const Msg &msg, StaticQueue<Msg, MSG_Q_SIZE> &outgoing_buffer);

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
    //TODO: This is so ugly in practice, just parent-class the type() data and change these apis
    //TODO: OR just override process() to accept different payloads
    size_t  process_srch(        AgentID from, const SrchPayload&, StaticQueue<Msg, MSG_Q_SIZE>&);
    size_t  process_srch_ret(    AgentID from, const SrchRetPayload&, StaticQueue<Msg, MSG_Q_SIZE>&);
    size_t  process_in_part(     AgentID from, const InPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&);
    size_t  process_ack_part(    AgentID from, const AckPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&);
    size_t  process_nack_part(   AgentID from, const NackPartPayload&, StaticQueue<Msg, MSG_Q_SIZE>&);
    size_t  process_noop( StaticQueue<Msg, MSG_Q_SIZE>& );
    //This does moderate lifting to determine if the search is complete for the
    //current node, and if so, returns the results to our leader
    size_t  check_search_status( StaticQueue<Msg, MSG_Q_SIZE>&);

    /* Join / Merge / Absorb stage message */
    //join_us does some heavy lifting to determine how partitions should be restructured and joined
    size_t  process_join_us(     AgentID from, const JoinUsPayload&, StaticQueue<Msg, MSG_Q_SIZE>&);
    //After a level change, we may have to do some cleanup responses, this will handle that.
    size_t  check_new_level( StaticQueue<Msg, MSG_Q_SIZE>& );

    bool                     index_of(const AgentID&, size_t& out_idx) const;
    void                     respond_later(const AgentID&, const InPartPayload &m);

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
