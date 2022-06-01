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
 * @file edge.h
 * @brief Edge structure definition and functions
 */

#ifndef GHS_EDGE
#define GHS_EDGE

#include "ghs/agent.h"
#include <limits>

/** 
 * LE
 */
namespace le{
  /** 
   * GHS 
   **/
  namespace ghs{

    /**
     * A typedef to hold the metrics. There's some work to do to make sure the metrics are:
     *
     *   * Symmetric (both peer and root agree on the value)
     *   * unique    (no two agents have the same metric)
     *
     * However, that is not captured here, and is handled at a "higher level". See unique_link_metric_to()
     * @see DemoComms::unique_link_metric_to()
     */
    typedef unsigned long metric_t;

    /**
     * This is the "worst" metric possible, defined simply as the maximum value reachable.
     *
     * We all need to agree on the worst metric, since we're all comparing against this to determine if we found a minimum weight outgoing edge. See mwoe()
     *
     * @see GhsState::mwoe()  for how it is used
     * @see DemoComms::unique_link_metric_to()  for why it is a size_t
     * @see DemoComms::little_iperf()
     */
    const metric_t WORST_METRIC=std::numeric_limits<metric_t>::max();

    /**
     * This is set to zero because metrics are usually zero initialized by default, and we do not want to have a bunch of zeros floating around in our search
     */
    const metric_t METRIC_NOT_SET=0;

    /// A status enumeration, for the ghs edges. 
    enum status_t {
      /// We have not probed this edge for information yet, or have not recieved a reponse
      UNKNOWN = 0,
      /// We have added this edge as an MST link
      MST     = 1,
      /// We have decided not to further consider this edge, either it was "bad", or it is already part of our partition
      DELETED =-1,
    };

    /**
     *
     * @brief A struct to hold all the communication edge information
     *
     */
    struct Edge
    {
      Edge(){}
      Edge(agent_t pe, agent_t ro, status_t st, metric_t me)
        : peer(pe), root(ro), status(st), metric_val(me) {}
      /// The peer is the "to" side of the edge
      agent_t   peer=NO_AGENT;
      /// The root is the "from" side of the edge.
      agent_t   root=NO_AGENT;
      /// The status of this edge, starting with UNKNOWN
      status_t  status=UNKNOWN;
      /// By default, this edge has metric_val = WORST_METRIC
      metric_t  metric_val=WORST_METRIC;
    };

    /// 
    /// Returns the worst possible edge, useful for comparisons in the search
    /// for the minimum weight outgoing edge.
    Edge worst_edge();

    /**
     * @param e an Edge
     * @return true if edge is not worst_edge() and does not have any uninitialized components.
     */
    bool is_valid(const Edge e);

    /**
     * @param m  a metric_t 
     * @return true if the metric_t is not `NOT_SET` or `WORST_METRIC`
     */
    bool is_valid(const metric_t m);
  }
}
#endif 
