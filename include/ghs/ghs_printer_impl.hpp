/**
 * @file ghs_printer_impl.hpp
 * @brief the implementation of ghs_printer.h Just include ghs_printer.h
 */
#include "ghs/ghs_printer.h"

#include <sstream>

using le::ghs::Edge;
using le::ghs::agent_t;
using le::ghs::UNKNOWN;
using le::ghs::MST;
using le::ghs::DELETED;
using le::ghs::OK;

/** 
 * Prints the edges in a neatly formatted way
 */
template <std::size_t A, std::size_t B>
std::string  dump_edges(const le::ghs::GhsState<A,B> &s) {
  std::stringstream ss;
  ss<<"( ";
  Edge mwoe = s.mwoe();
  ss<<" m:"<<mwoe.peer;
  ss<<" mw:"<<mwoe.metric_val;
  for (std::size_t i=0;i<A;i++){
    if (s.has_edge(i)){
      Edge e;
      if (OK!=s.get_edge((agent_t)i,e)){
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


/**
 * Prints some basic GHS information, like leader, level, # waiting, # delayed, converged (yes/no)
 */
template <std::size_t A, std::size_t B>
std::ostream& operator << ( std::ostream& outs, const le::ghs::GhsState<A,B> & s){
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

