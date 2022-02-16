#ifndef graph_hpp
#define graph_hpp

#include <stdlib.h> //size_t

#ifndef GHS_GRAPH_WORST_EDGE_METRIC
#define GHS_GRAPH_WORST_EDGE_METRIC 99999999
#endif

typedef size_t AgentID;
typedef size_t Level;

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
typedef struct {
  AgentID    peer;//to
  AgentID    root;//from
  EdgeStatus status;
  //Lower is better
  size_t        metric_val;
} Edge;

/**
 * For comparison
 */
Edge ghs_worst_possible_edge();

#endif
