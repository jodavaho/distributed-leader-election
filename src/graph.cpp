#include "graph.hpp"

Edge ghs_worst_possible_edge(){
  Edge ret;
  ret.root=-1;
  ret.peer=-1;
  ret.status=UNKNOWN;
  ret.metric_val=GHS_GRAPH_WORST_EDGE_METRIC;
  return ret;
}
