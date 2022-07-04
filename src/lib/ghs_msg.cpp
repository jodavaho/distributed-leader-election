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
 * @file msg.cpp
 *
 */
#include "ghs/msg.h"

namespace le{
  namespace ghs{

    using namespace msg;

    Msg::Msg(): to_(NO_AGENT), from_(NO_AGENT), type_(UNASSIGNED)
    {}
    Msg::Msg(agent_t to, agent_t from, NoopPayload p):to_(to),from_(from){
      type_=NOOP;
      data_.noop = p;
    }
    Msg::Msg(agent_t to, agent_t from, SrchPayload p):to_(to),from_(from){
      type_=SRCH;
      data_.srch=p;
    }
    Msg::Msg(agent_t to, agent_t from, SrchRetPayload p):to_(to),from_(from){
      type_=SRCH_RET;
      data_.srch_ret=p;
    }
    Msg::Msg(agent_t to, agent_t from, InPartPayload p):to_(to),from_(from){
      type_=IN_PART;
      data_.in_part=p;
    }
    Msg::Msg(agent_t to, agent_t from, AckPartPayload p):to_(to),from_(from){
      type_=ACK_PART;
      data_.ack_part=p;
    }
    Msg::Msg(agent_t to, agent_t from, NackPartPayload p):to_(to),from_(from){
      type_=NACK_PART;
      data_.nack_part=p;
    }
    Msg::Msg(agent_t to, agent_t from, JoinUsPayload p):to_(to),from_(from){
      type_=JOIN_US;
      data_.join_us=p;
    }
    Msg::Msg(agent_t to, agent_t from, const Msg &other):to_(to),from_(from){
      type_=other.type();
      data_=other.data();
    }
    Msg::Msg(agent_t to, agent_t from, Type t, Data d):to_(to),from_(from){
      type_=t;
      data_=d;
    }
    Msg::~Msg()
    { }

  }
}
