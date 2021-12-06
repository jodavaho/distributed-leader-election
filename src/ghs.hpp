#ifndef GHS_HPP
#define GHS_HPP

#include <unordered_set>
#include <set>
#include <array>
#include <deque>
#include "msg.hpp"

typedef  int  AgentID;
typedef  int  GHS_STATUS;

extern GHS_STATUS  GHS_OK;
extern GHS_STATUS  GHS_ERR;

typedef enum {
  UNKNOWN = 0,
  MST     = 1,
  DELETED =-1,
} EdgeStatus;

typedef struct {
  AgentID    root;
  AgentID    peer;
  EdgeStatus status;
  //Lower is better
  int        metric_val;
} Edge;

Edge ghs_worst_possible_edge();

typedef struct {
  AgentID leader;
  int     level;
} Partition;

class GHS_State
{
  public:

    GHS_State(AgentID my_id, int num_agents, std::vector<Edge> edges={}) noexcept;

    //setters
    int set_edge(const Edge &e) noexcept;
    void set_partition(const Partition& p) noexcept;
    void set_parent_edge(const Edge&e);

    //getters
    size_t waiting_count() const noexcept;
    Edge mwoe() const noexcept;

    //useful operations
    /**
     * Sends to MST child links only
     * @return number of messages sent
     */
    size_t mst_broadcast(const Msg::Type &msgtype, const std::vector<int> data, std::deque<Msg> *buf) const noexcept;

    /**
     * Sends to parent MST link only
     * @return number of mssages sent (it had better be 1 or 0 if this is a root)
     */
    size_t mst_convergecast(const Msg::Type &msgtype, const std::vector<int>data, std::deque<Msg>*buf)const noexcept;

    /**
     * Filters edges by `msgtype`, and sends outgoing message along those that match.
     * @return number of messages sent
     */
    size_t typecast(const EdgeStatus& status, const Msg::Type &m, const std::vector<int> data, std::deque<Msg> *buf) const noexcept;

    //stateful algorithm steps
    void start_round(std::deque<Msg> *outgoing_msgs) noexcept;
    int process(const Msg &msg, std::deque<Msg> *outgoing_buffer);

    //reset the algorithm state
    bool reset() noexcept;


  private:
    int  process_srch(        AgentID from, std::vector<int> data, std::deque<Msg>*);
    int  process_srch_ret(    AgentID from, std::vector<int> data, std::deque<Msg>*);
    int  process_in_part(     AgentID from, std::vector<int> data, std::deque<Msg>*);
    int  process_ack_part(    AgentID from, std::vector<int> data, std::deque<Msg>*);
    int  process_nack_part(   AgentID from, std::vector<int> data, std::deque<Msg>*);
    int  process_join_us(     AgentID from, std::vector<int> data, std::deque<Msg>*);
    int  process_election(    AgentID from, std::vector<int> data, std::deque<Msg>*);
    int  process_new_sheriff( AgentID from, std::vector<int> data, std::deque<Msg>*);
    int  process_not_it(      AgentID from, std::vector<int> data, std::deque<Msg>*);

    int                          num_agents;
    AgentID                      my_id;
    Partition                    my_part;
    AgentID                      parent;
    std::unordered_set<AgentID>  waiting_for;
    std::vector<Edge>            outgoing_edges;
    Edge                         best_edge;

};

#endif
