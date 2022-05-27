#ifndef graph_hpp
#define graph_hpp

#include <limits>

//The algorithm "level" for partitions of the graph
#define GHS_LEVEL_START 0
typedef int Level;

//TODO: namespace ghs{
#define GHS_NO_AGENT -1
typedef unsigned int AgentID;
bool ghs_agent_is_valid(const AgentID&);

#define GHS_EDGE_METRIC_NOT_SET 0
#ifndef GHS_EDGE_WORST_METRIC
#define GHS_EDGE_WORST_METRIC std::numeric_limits<EdgeMetric>::max()
#endif

typedef unsigned long EdgeMetric;
bool ghs_metric_is_valid(const EdgeMetric&);

typedef enum {
  UNKNOWN = 0,
  MST     = 1,
  DELETED =-1,
} EdgeStatus;


/*
 * There's a todo here.
 * metric_val needs to be unique, and fully comparable.
 *
 * I think the way to do that is to make metric_val = rssi << n + agent_1 << m + agent_2
 *
 * n = 2*log( num_agents) 
 * m = log(num_agents)
 *
 * So, an edge with weight 4, going from 0 to 1 on a 4-agent system would have
 * binary metric of 100001, so we're basically comparing weights, unless
 * weights are equal, in which case we break ties by the two agents on the
 * edge, lower id taking priority over higher. 
 *
 * Alternatively, just implement <=> operator using that information. 
 */ 
struct Edge{
  Edge(AgentID p=GHS_NO_AGENT, AgentID r=GHS_NO_AGENT, EdgeStatus e=UNKNOWN, EdgeMetric m=GHS_EDGE_WORST_METRIC): 
    peer(p), root(r), status(e), metric_val(m) {}
  AgentID    peer;
  AgentID    root;
  EdgeStatus status;
  //Lower is better
  EdgeMetric metric_val;
  bool is_valid();
};

//we sometimes pass around invalid edges to say "no such edge exists"
bool ghs_edge_is_valid(const Edge &e);

/**
 * For comparison, an edge that is both invalid, and
 * "larger than" all other edges (by metric_val)
 */
Edge ghs_worst_possible_edge();

#endif
