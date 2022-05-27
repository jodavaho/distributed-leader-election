#include "ghs-demo-edgemetrics.hpp"
#include <algorithm>

CommsEdgeMetric sym_metric(
    const uint16_t agent_to,
    const uint16_t agent_from,
    const Kbps kbps)
{
  uint16_t bigger = (uint16_t) std::max((int)agent_to, (int)agent_from);
  uint16_t smaller = (uint16_t) std::min((int)agent_to, (int)agent_from);
  CommsEdgeMetric ikbps=kbps;
  //how long would it take to send lots of data over this link?
  if (ikbps==0){ //or, if ikbps == 1, really
    ikbps=std::numeric_limits<CommsEdgeMetric>::max();//too long
  } else {
    ikbps =(CommsEdgeMetric)  ((double) std::numeric_limits<CommsEdgeMetric>::max() / (double) kbps);//not so long (min=1 "unit")
  }
  return 
     ((uint64_t)bigger<<16)
    +((uint64_t)smaller<<0)
    +((uint64_t)ikbps <<32) 
    ;
}
