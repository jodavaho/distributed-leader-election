#ifndef GHS_HPP
#define GHS_HPP

#include <unordered_set>
#include <memory>  //shared_ptr
#include <set>
#include <array>
#include <deque>
#include "msg.hpp"
#include <optional>

typedef size_t AgentID;

typedef enum {
  UNKNOWN = 0,
  MST     = 1,
  DELETED =-1,
} EdgeStatus;

typedef struct {
  AgentID    peer;//to
  AgentID    root;//from
  EdgeStatus status;
  //Lower is better
  size_t        metric_val;
} Edge;

Edge ghs_worst_possible_edge();

typedef struct {
  AgentID leader;
  size_t     level;
} Partition;

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
    GhsState(AgentID my_id,  std::vector<Edge> edges={}) noexcept;

    /**
     * Changes (or adds) an edge to the outgoing edge list.  If the edge you
     * pass in does not exist, then it will be added.  Edges are considered
     * identical only if they are to-and-from the same nodes. 
     *
     * @throws invalid_argument if you attempt to add an edge
     * that is rooted on another node. 
     *
     *
     * @Return: 0 if edge updated. 1 if it was inserted (it's new)
     */
    size_t set_edge(const Edge &e);

    /**
     * Changes the edge status of the edge connecting this node to the AgentID given
     *
     * @throws invalid_argument if you attempt to modify an edge that doesn't
     * exist. 
     */
    void set_edge_status(const AgentID &to, const EdgeStatus &status);

    /**
     * Returns the outgoing edge to the AgentID you request, if it exists
     */
    std::optional<Edge> get_edge(const AgentID& to) noexcept;

    /**
     * Sets the partition that this node is part of.
     *
     * A Partition has two parts: a Leader ID and a "level" which is an
     * internal variable used by the algorithm. 
     */
    void set_partition(const Partition& p) noexcept;

    /**
     *
     * Gets the partition information. Never fails to return.
     */
    Partition get_partition() const noexcept;

    /**
     * Returns whatever was set (or initialized) as the AgentID
     *
     * Never fails to return
     */
    AgentID get_id() const noexcept;

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
    void set_parent_edge(const Edge&e);

    //getters
    size_t waiting_count() const noexcept;
    size_t delayed_count() const noexcept;

    Edge mwoe() const noexcept;

    //useful operations
    /**
     * Sends to MST child links only
     * @return number of messages sent
     */
    size_t mst_broadcast(const Msg::Type &msgtype, const std::vector<size_t> data, std::deque<Msg> *buf) const noexcept;

    /**
     * Sends to parent MST link only
     * @return number of mssages sent (it had better be 1 or 0 if this is a root)
     */
    size_t mst_convergecast(const Msg::Type &msgtype, const std::vector<size_t>data, std::deque<Msg>*buf)const noexcept;

    /**
     * Filters edges by `msgtype`, and sends outgoing message along those that match.
     * @return number of messages sent
     */
    size_t typecast(const EdgeStatus& status, const Msg::Type &m, const std::vector<size_t> data, std::deque<Msg> *buf) const noexcept;

    //stateful algorithm steps
    void start_round(std::deque<Msg> *outgoing_msgs) noexcept;
    size_t process(const Msg &msg, std::deque<Msg> *outgoing_buffer);

    //reset the algorithm state
    bool reset() noexcept;


  private:

    /* Search stage messages */
    //each of srch, srch_ret, in_part, ack_part, nack_part are deterministic and straightfoward
    size_t  process_srch(        AgentID from, std::vector<size_t> data, std::deque<Msg>*);
    size_t  process_srch_ret(    AgentID from, std::vector<size_t> data, std::deque<Msg>*);
    size_t  process_in_part(     AgentID from, std::vector<size_t> data, std::deque<Msg>*);
    size_t  process_ack_part(    AgentID from, std::vector<size_t> data, std::deque<Msg>*);
    size_t  process_nack_part(   AgentID from, std::vector<size_t> data, std::deque<Msg>*);
    //This does moderate lifting to determine if the search is complete for the
    //current node, and if so, returns the results to our leader
    size_t  check_search_status( std::deque<Msg>*);

    /* Join / Merge / Absorb stage message */
    //join_us does some heavy lifting to determine how partitions should be restructured and joined
    size_t  process_join_us(     AgentID from, std::vector<size_t> data, std::deque<Msg>*);
    //new_sheriff is the msg that initiates a restructure, and is completely deterministic
    size_t  process_new_sheriff( AgentID from, std::vector<size_t> data, std::deque<Msg>*);
    //After a level change, we may have to do some cleanup responses, this will handle that.
    size_t  check_new_level( std::deque<Msg>* );

    /* Leader election messages */
    //these are kept here for completeness, but are only required 
    size_t  process_election(    AgentID from, std::vector<size_t> data, std::deque<Msg>*);
    size_t  process_not_it(      AgentID from, std::vector<size_t> data, std::deque<Msg>*);



    AgentID                      my_id;
    Partition                    my_part;
    AgentID                      parent;
    std::unordered_set<AgentID>  waiting_for;
    std::vector<Edge>            outgoing_edges;
    std::vector<Msg>             respond_later;
    Edge                         best_edge;
    Partition                    best_partition;

};

#endif
