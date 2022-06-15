/**
 *   @copyright 
 *   Copyright (c) 2022 California Institute of Technology (“Caltech”). 
 *   U.S.  Government sponsorship acknowledged.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *    * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    *  Neither the name of Caltech nor its operating division, the Jet
 *    Propulsion Laboratory, nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file local_graph.h
 * @brief provides le::LocalGraph defintion
 *
 */
#ifndef LOCAL_GRAPH_H
#define LOCAL_GRAPH_H 

namespace le{

  template<unsigned int SZ>
  class LocalGraph{
    public:

      LocalGraph()
        :sz_(0){}

      struct LocalEdge
      {
        unsigned int to;
        unsigned int status;
        unsigned int weight;
      };

      enum class Errno{
        OK=0,
        DOES_NOT_EXIST,
        ALREADY_EXISTS
      };

      unsigned int size(){return sz_;}
      unsigned int max_size(){ return SZ; }

      Errno set(const unsigned int to, const unsigned int status, const unsigned int weight);
      Errno get(unsigned int & to, unsigned int & status, unsigned int & weight) const;

      Errno set(unsigned int to, const LocalEdge e);
      Errno get(unsigned int to, LocalEdge &out_e) const;

      Errno set_weight(const unsigned int to, const unsigned int wt);
      Errno set_status(const unsigned int to, const unsigned int status);

      Errno get_weight(const unsigned int to, unsigned int &out_wt)const;
      Errno get_status(const unsigned int to, unsigned int &out_status)const;

      static const char * strerror(const Errno e);

    private:

      unsigned int sz_;
      LocalEdge links[SZ];

  };
}

#endif

