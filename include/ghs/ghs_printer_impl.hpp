#include "ghs_printer.hpp"

#include <sstream>

template <std::size_t A, std::size_t B>
std::string  dump_edges(const GhsState<A,B> &s) {
  std::stringstream ss;
  ss<<"( ";
  for (std::size_t i=0;i<A;i++){
    if (s.has_edge(i)){
      Edge e;
      s.get_edge(i,e);
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

template <std::size_t A, std::size_t B>
std::ostream& operator << ( std::ostream& outs, const GhsState<A,B> & s){
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

