#include "ghs/edge.h"

namespace le{
  namespace ghs{

    Edge worst_edge(){
      Edge ret;
      ret.root=NO_AGENT;
      ret.peer=NO_AGENT;
      ret.status=UNKNOWN;
      ret.metric_val=WORST_METRIC;
      return ret;
    }

    bool is_valid(const metric_t m){
      return m!=METRIC_NOT_SET
        && m!=WORST_METRIC;
    }

    bool is_valid(const Edge e){
      return is_valid(e.peer)
        && is_valid(e.root)
        && is_valid(e.metric_val);
    }

  }
}
