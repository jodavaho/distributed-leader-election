/**
 * @copyright 
 * Copyright (c) 2022 California Institute of Technology (“Caltech”). 
 * U.S.  Government sponsorship acknowledged.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *   *  Neither the name of Caltech nor its operating division, the Jet
 *   Propulsion Laboratory, nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @file ghs_printer_impl.hpp
 * @brief the implementation of ghs_printer.h Just include ghs_printer.h
 *
 */
#include <ghs/ghs_printer.h>
#include <sstream>

/** 
 * Prints the edges in a neatly formatted way
 */
template <std::size_t A, std::size_t B>
std::string  dump_edges(const dle::GhsState<A,B> &s) {

  using dle::Edge;

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
        case MST: {ss<<"MST+C ";break;}
        case MST_PARENT: {ss<<"MST+P ";break;}
        case DELETED: {ss<<"DEL";break;}
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
std::ostream& operator << ( std::ostream& outs, const dle::GhsState<A,B> & s){
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

