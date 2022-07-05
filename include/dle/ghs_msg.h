/**
 *
 *
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
 * @file ghs_msg.h
 * @brief provides the defs for the struct dle::GhsMsg and GhsMsg namespace
 *
 */

#ifndef DLE_GHSMSG
#define DLE_GHSMSG

#include "agent.h"
#include "level.h"
#include "edge.h"

/**
*/
namespace dle{
  namespace ghs_msg{
    /// Stores what type of GhsMsg this is
    enum Type
    {
      UNASSIGNED=0,///< Error checking for unassigned messages
      NOOP,///< data is a NoopPayload
      SRCH,///< data is a SrchPayload
      SRCH_RET,///< data is a SrchRetPayload
      IN_PART,///< data is a InPartPayload
      ACK_PART,///< data is a AckPartPayload
      NACK_PART,///< data is a NackPartPayload
      JOIN_US,///< data is a JoinUsPayload
    };

    /// No further action necessary (i.e., we have completed the MST construction)
    struct NoopPayload{ 
    }; 

    /// Requests a search begin in the MST subtree rooted at the receiver, for the minimum weight outgoing edge (one that spans two partitions).
    struct SrchPayload{
      agent_t your_leader;
      level_t   your_level;
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
    };

    /// Asks "Are you in my partition"
    struct InPartPayload{
      agent_t leader;
      level_t   level; 
    };

    /// States "I am in your partition"
    struct AckPartPayload{ 
    };

    /// States "I am not in your partition"
    struct NackPartPayload{ 
    };

    /** 
     * @brief GhsMsgs to merge /absorb two partitions across a given edge
     *
     */
    struct JoinUsPayload{ 
      agent_t join_peer;
      agent_t join_root;
      agent_t proposed_leader;
      level_t proposed_level;
    };

    union Data{
      NoopPayload noop;
      SrchPayload srch;
      SrchRetPayload srch_ret;
      InPartPayload in_part;
      AckPartPayload ack_part;
      NackPartPayload nack_part;
      JoinUsPayload join_us;
    };


    /** 
     * @brief An aggregate type containing all the data to exchange with to/from information
     *
     * The GhsMsg struct contains all the data which is passed between GhsState
     * objects operating on different systems to coordinate the construction
     * of an MST.
     *
     * In the example case, the field `other_guy` is used twice, but that may
     * not be the case always. to_msg takes the extra step of setting the
     * to/from fields appropriately and seperate from the payload fields. I
     * specifically made this design design to defend myself from myself after
     * messing up the data fields far too often. 
     */
    class GhsMsg
    {

      public:

        GhsMsg();

        /** 
         * @brief A type-specific constructor for each payload type
         */
        GhsMsg(agent_t to, agent_t from, ghs_msg::NoopPayload p);
        GhsMsg(agent_t to, agent_t from, ghs_msg::SrchPayload p);
        GhsMsg(agent_t to, agent_t from, ghs_msg::SrchRetPayload p);
        GhsMsg(agent_t to, agent_t from, ghs_msg::InPartPayload p);
        GhsMsg(agent_t to, agent_t from, ghs_msg::AckPartPayload p);
        GhsMsg(agent_t to, agent_t from, ghs_msg::NackPartPayload p);
        GhsMsg(agent_t to, agent_t from, ghs_msg::JoinUsPayload p);

        /**
         * A 'redirect' constructor that perserves type and payload, but allows new to/from fields
         */
        GhsMsg(agent_t to, agent_t from, const GhsMsg &other);

        /**
         * A generic constructor for generic data, and known type
         */
        GhsMsg(agent_t to, agent_t from, ghs_msg::Type t, ghs_msg::Data d);

        ~GhsMsg();

        agent_t to() const {return to_;}
        agent_t from() const {return from_;}
        ghs_msg::Type type() const {return type_;}
        ghs_msg::Data data() const {return data_;}

      private:
        /// who to send to
        agent_t to_; 

        /// who it is from
        agent_t from_;

        ghs_msg::Data data_;

        ghs_msg::Type type_;

    };

    /**
     * For an external class that is interested in allocating static storage
     * to queue a set of GhsMsg s, this is the maximum size of the GhsMsg class. 
     */
    const unsigned int MAX_MSG_SZ= sizeof(GhsMsg);
  }

}

#endif
