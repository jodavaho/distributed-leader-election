#include "ghs_printer.hpp"

#include <sstream>

template <std::size_t A, std::size_t B>
std::string  dump_edges(const GhsState<A,B> &s) {
  std::stringstream ss;
  ss<<"( ";
  Edge mwoe = s.mwoe();
  ss<<" m:"<<mwoe.peer;
  ss<<" mw:"<<mwoe.metric_val;
  for (std::size_t i=0;i<A;i++){
    if (s.has_edge(i)){
      Edge e;
      if (GHS_OK!=s.get_edge((AgentID)i,e)){
        ss<<"Err: "<<i;
        continue;
      };
      ss<<" ";
      ss<<e.root<<"-->"<<e.peer<<" ";
      switch (e.status){
        case UNKNOWN: {ss<<"UNK";break;}
        case MST: {ss<<"MST";break;}
        case DELETED: {ss<<"DEL";break;}
      }
      if (e.peer == s.get_parent_id()){
        ss<<"+P";
      } else if (e.status==MST){
        ss<<"+C";
      } else {
        ss<<" _";
      }
      if (e.peer == mwoe.peer){
        ss<<"m";
      } else {
        ss<<"_";
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

