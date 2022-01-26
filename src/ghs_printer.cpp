#include "ghs_printer.hpp"

#include <sstream>

std::string  dump_edges(const GhsState &s) {
  std::stringstream ss;
  ss<<"( ";
  for (int i=0;i<GHS_MAX_AGENTS;i++){
    if (s.has_edge(i)){
      Edge e =s.get_edge(i);
      ss<<" ";
      ss<<e.root<<"-->"<<e.peer<<" ";
      switch (e.status){
        case UNKNOWN: {ss<<"---";break;}
        case MST: {ss<<"MST";break;}
        case DELETED: {ss<<"DEL";break;}
      }
      if (e.peer == s.get_parent_id()){
        ss<<"+P";
      } else {
        ss<<"  ";
      }
      ss<<" "<<e.metric_val<<";";
    }
  }
  ss<<")";
  return ss.str();
}

std::ostream& operator << ( std::ostream& outs, const GhsState & s){
  outs<<"{id:"<<s.get_id()<<" ";
  outs<<"leader:"<<s.get_leader_id()<<" ";
  outs<<"level:"<<s.get_level()<<" ";
  outs<<"waiting:"<<s.waiting_count()<<" ";
  outs<<"delayed:"<<s.delayed_count()<<" ";
  outs<<"converged:"<<s.is_converged()<<" ";
  outs<<"("<<dump_edges(s)<<")";
  outs<<"}";
  return outs;
}

