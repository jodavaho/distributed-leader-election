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
 * @file errno.h
 * @brief Provides error codes for all classes / functions
 *
 */

#ifndef LE_ERRNO_H
#define LE_ERRNO_H

namespace le{
  /**
   * The Errno enumeration used by all classes for all function calls
   * that may fail. 
   */
  enum Errno{
    OK = 0,                    ///< The operation was successful
    PROCESS_SELFMSG,           ///< Could not process a message from self
    PROCESS_NOTME,             ///< Could not process a message not directed towards this agent
    PROCESS_INVALID_TYPE,      ///< Did not recognize or could not process this type of message
    PROCESS_REQ_MST,           ///< This type of message (possibly only at this time) should have come over and MST link, but did not
    SRCH_INAVLID_SENDER,       ///< There is no reason to expect a SRCH message from this sender (usually parent and MST link required)
    SRCH_STILL_WAITING,        ///< THere is no way to process a SRCH message when we're still executing the last search ( `waiting_count()` > 0 )
    ERR_QUEUE_MSGS,            ///< Unable to enqueue messages, received seque::Retcode not OK
    PROCESS_NO_EDGE_FOUND,     ///< Unable to process the message when we don't have an edge to that agent
    UNEXPECTED_SRCH_RET,       ///< Unexpected srch_ret message at this time (not searching or not waiting for that agent)
    ACK_NOT_WAITING,           ///< We cannot process an ACK message if we aren't expecting one
    BAD_MSG,                   ///< Likely malformed message
    JOIN_BAD_LEADER,           ///< Received join message with a leader not our own, yet we are not on a partition boundary
    JOIN_BAD_LEVEL,            ///< Received join message with a non-matching level, yet we received join msg with different level
    JOIN_INIT_BAD_LEADER,      ///< Told to init join to another parition, but leader unrecognized
    JOIN_INIT_BAD_LEVEL,       ///< Told to init join to another partition, but level unrecognized
    JOIN_MY_LEADER,            ///< Other partition suggested we join our own partition
    JOIN_UNEXPECTED_REPLY,     ///< received higher-level join message: Impossible since we should not have replied to their SRCH yet
    ERR_IMPL,                  ///< Implementation error: Reached branch that should not have been reachable
    CAST_INVALID_EDGE,         ///< *cast operation failed because of bad edge
    SET_INVALID_EDGE,          ///< add- or set edge failed because of malformed edge
    SET_INVALID_EDGE_METRIC,   ///< add- or set edge failed because metric_val==WORST_METRIC
    SET_INVALID_EDGE_NOT_ROOT, ///< add- or set edge failed because edge was not rooted on my_id
    SET_INVALID_EDGE_NO_AGENT, ///< add- or set edge failed because of NO_AGENT as peer/root
    SET_INVALID_EDGE_SELF_LOOP, ///< add- or set edge failed because Edge.peer == my_id
    PARENT_UNRECOGNIZED,       ///< Cannot set parent ID to unrecognized node (no edge to them!)
    PARENT_REQ_MST,            ///< Cannot set parent ID to non-MST node (bad edge type)
    NO_SUCH_PEER,              ///< Cannot find peer idx -- no edge or unrecognized ID?
    IMPL_REQ_PEER_MY_ID,       ///< Cannot treat my_id as peer -- bad message?
    TOO_MANY_AGENTS,           ///< Set- or add edge failed, too many agents in static storage already
    PARTIAL_RESULT,            ///< When returning the result, this means we have not converged yet
    NO_AGENTS,                 ///< Operation failed because we have no outgoing edges
    ERR_QUEUE_FULL,///< Operation failed, the queue is full
    ERR_QUEUE_EMPTY,///< Operation failed, the queue is empty
    ERR_BAD_IDX,///< Operation failed and is not possible to succeed: that idx is beyond the static size of the queue
    ERR_NO_SUCH_ELEMENT,///< Operation failed, there are less elements than the given index in the queue
  };

  /**
   * @return a human-readable string for any value of the passed in Retcode
   * @param r a le::Errnoo
   */
  const char* strerror( const Errno e);

}

#endif
