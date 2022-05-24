#include "ghs/graph.hpp"

Edge ghs_worst_possible_edge(){
  Edge ret;
  ret.root=GHS_NO_AGENT;
  ret.peer=GHS_NO_AGENT;
  ret.status=UNKNOWN;
  ret.metric_val=GHS_EDGE_WORST_METRIC;
  return ret;
}

bool ghs_agent_is_valid(const AgentID &id)
{
  return (id>=0);
}

bool ghs_metric_is_valid(const EdgeMetric &m){
    return m!=GHS_EDGE_METRIC_NOT_SET
    && m!=GHS_EDGE_WORST_METRIC;
}

bool ghs_edge_is_valid(const Edge &e){
  return ghs_agent_is_valid(e.peer)
    && ghs_agent_is_valid(e.root)
    && ghs_metric_is_valid(e.metric_val);
}

bool Edge::is_valid(){
  return ghs_agent_is_valid(peer)
    && ghs_agent_is_valid(root)
    && ghs_metric_is_valid(metric_val);
}
