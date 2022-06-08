/**
 * @copyright
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
 * @file errno.cpp
 * @brief Provides implementation of functions related to error codes 
 *
 */

#include "le/errno.h"

namespace le{
  const char* strerror( const le::Errno e){
    switch (e){
      case OK:{return "No Error";}
      case PROCESS_SELFMSG:{return "Cannot process msg from self";}
      case PROCESS_NOTME:{return "Cannot process msg to a diff. agent!";}
      case PROCESS_INVALID_TYPE:{return "Do not know how to process this type of msg";}
      case PROCESS_REQ_MST:{return "This msg did not come from MST link, as it should have";}
      case SRCH_INAVLID_SENDER:{return "Cannot process SRCH msg from this agent. Need valid tree edge (or self-sent)";}
      case SRCH_STILL_WAITING:{return "Received srch msg from parent, while still waiting for replies -- new round?";}
      case ERR_QUEUE_MSGS:{return "The outgoing queue did not capture all our msgs";}
      case PROCESS_NO_EDGE_FOUND:{return "do not have an edge to the sender!";}
      case UNEXPECTED_SRCH_RET:{return "received a srch_ret call, but were not expecting it";}
      case ACK_NOT_WAITING:{return "received ACK from an agent we were not waiting for";}
      case BAD_MSG:{return "A malformed msg was received";}
      case JOIN_BAD_LEADER:{return "Not on parition boundary, and received join_us from unrecognized leader";}
      case JOIN_BAD_LEVEL:{return "Not on partition boundary, and rec'd join_us with a different level";}
      case JOIN_INIT_BAD_LEADER:{return "Unrecognized leader id when initiating join with another partition";}
      case JOIN_INIT_BAD_LEVEL:{return "Different/incorrect level when initiating join with another partition";}
      case JOIN_MY_LEADER:{return "other partition suggested I join my partition";}
      case JOIN_UNEXPECTED_REPLY:{return "rec higher-level join_us: impossible since we should not have replied to their in_part yet";}
      case ERR_IMPL: {return "Developer error: Reached 'unreachable' branch";}
      case CAST_INVALID_EDGE:{return "*cast operation failed with bad outgoing edge";}
      case SET_INVALID_EDGE:{return "set/add edge operation failed b/c of bad edge";}
      case PARENT_UNRECOGNIZED:{return "cannot set parent to unrecognized node (no edge)";}
      case PARENT_REQ_MST:{return "cannot set parent to non-MST node (bad edge type)";}
      case NO_SUCH_PEER:{return "Cannot find peer idx -- no edge or unrecognized id?";}
      case IMPL_REQ_PEER_MY_ID:{return "Cannot treat my_id as peer -- likely implementation error or malformed msg";}
      case TOO_MANY_AGENTS:{return "set/add edge operation failed b/c too many agents";}
      case ERR_QUEUE_FULL: { return "Queue FULL"; }
      case ERR_QUEUE_EMPTY: { return "Queue EMPTY"; }
      case ERR_BAD_IDX: { return "Queue IDX >= underlying buffer size"; }
      case ERR_NO_SUCH_ELEMENT: { return "Queue IDX >= size()"; }
      case PARTIAL_RESULT: { return "The algorithm has not converged!"; }
      case NO_AGENTS: { return "The algorithm has not converged!"; }
      // DO NOT ADD DEFAULT or you lose compile-time checks for new error codes.
    }
    return "You should not see this message (errno.cpp)";
  }
}
