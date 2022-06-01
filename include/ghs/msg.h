/**
 *
 *
 *   Copyright (c) 2021 California Institute of Technology (“Caltech”). 
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
 * @file msg.h
 * @brief provides the defs for the struct le::ghs::msg 
 *
 */
#ifndef GHS_MSG_H
#define GHS_MSG_H

#include "ghs/agent.h"
#include "ghs/level.h"
#include "ghs/edge.h"

/**
*/
namespace le{
  /**
   *
   */
  namespace ghs{

    /** 
     * @brief An aggregate type containing all the data to exchange with to/from information
     *
     * The Msg struct contains all the data which is passed between GhsState
     * objects operating on different systems to coordinate the construction
     * of an MST.
     *
     * The usual way to construct a Msg is to construct the payload from a struct of type Msg::Data, then to call to_msg() on that payload.
     *
     */
    struct Msg;

    /// No further action necessary (i.e., we have completed the MST construction)
    struct NoopPayload{ 
      Msg to_msg(agent_t to, agent_t from);
    }; 

    /// Requests a search begin in the MST subtree rooted at the receiver, for the minimum weight outgoing edge (one that spans two partitions).
    struct SrchPayload{
      agent_t your_leader;
      level_t   your_level;
      Msg to_msg(agent_t to, agent_t from);
    };

    /** 
     * @brief Returns an edge that represents the minimum weight outgoing edge
     *
     * Returns an edge that represents the minimum weight outgoing edge (across paritions) from the MST subtree rooted at the sender.
     */
    struct SrchRetPayload{
      agent_t to;
      agent_t from;
      metric_t metric;
      Msg to_msg(agent_t to, agent_t from);
    };

    /// Asks "Are you in my partition"
    struct InPartPayload{
      agent_t leader;
      level_t   level; 
      Msg to_msg(agent_t to, agent_t from);
    };

    /// States "I am in your partition"
    struct AckPartPayload{ 
      Msg to_msg(agent_t to, agent_t from);
    };

    /// States "I am not in your partition"
    struct NackPartPayload{ 
      Msg to_msg(agent_t to, agent_t from);
    };

    /** 
     * @brief Msgs to merge /absorb two partitions across a given edge
     *
     */
    struct JoinUsPayload{ 
      agent_t join_peer;
      agent_t join_root;
      agent_t proposed_leader;
      level_t proposed_level;
      Msg to_msg(agent_t to, agent_t from);
    };


    /** 
     * @brief An aggregate type containing all the data to exchange with to/from information
     *
     * The Msg struct contains all the data which is passed between GhsState
     * objects operating on different systems to coordinate the construction
     * of an MST.
     *
     * The usual way to construct a Msg is to construct the payload from a struct of type Msg::Data, then to call to_msg() on that payload.
     *
     * In the example case, the field `other_guy` is used twice, but that may
     * not be the case always. to_msg takes the extra step of setting the
     * to/from fields appropriately and seperate from the payload fields. I
     * specifically made this design design to defend myself from myself after
     * messing up the data fields far too often. 
     */
    struct Msg
    {

      /** 
       * @brief A default constructor
       *
       * The usual way to construct a Msg is to construct the payload from a struct of type Msg::Data, then to call to_msg() on that payload.
       *
       */
      Msg();
      ~Msg();

      /// who to send to
      agent_t to; 

      /// who it is from
      agent_t from;

      /// Stores what type of Msg this is
      enum Type
      {
        NOOP=0,///< data is a NoopPayload
        SRCH,///< data is a SrchPayload
        SRCH_RET,///< data is a SrchRetPayload
        IN_PART,///< data is a InPartPayload
        ACK_PART,///< data is a AckPartPayload
        NACK_PART,///< data is a NackPartPayload
        JOIN_US///< data is a JoinUsPayload
      } type;

      /// A union of all possible payloads
      union Data{
        NoopPayload noop;
        SrchPayload srch;
        SrchRetPayload srch_ret;
        InPartPayload in_part;
        AckPartPayload ack_part;
        NackPartPayload nack_part;
        JoinUsPayload join_us;
      } data;
    };

    /**
     * For an external class that is interested in allocating static storage
     * to queue a set of Msg s, this is the maximum size of the Msg class. 
     */
    const long long int MAX_MSG_SZ= sizeof(Msg);

  }
}

#endif
