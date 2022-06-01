/**
 *   @copyright 
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
 * @file msg.cpp
 *
 */
#include "ghs/msg.h"

namespace le{
  namespace ghs{

    Msg SrchPayload::to_msg(agent_t to, agent_t from){
      Msg m;
      m.to=to;
      m.from=from;
      m.type=Msg::Type::SRCH;
      m.data.srch = *this;
      return m;
    }

    Msg SrchRetPayload::to_msg(agent_t to, agent_t from){
      Msg m;
      m.to=to;
      m.from=from;
      m.type=Msg::Type::SRCH_RET;
      m.data.srch_ret = *this;
      return m;
    }


    Msg InPartPayload::to_msg(agent_t to, agent_t from){
      Msg m;
      m.to=to;
      m.from=from;
      m.type=Msg::Type::IN_PART;
      m.data.in_part= *this;
      return m;
    }

    Msg AckPartPayload::to_msg(agent_t to, agent_t from){
      Msg m;
      m.to=to;
      m.from=from;
      m.type=Msg::Type::ACK_PART;
      m.data.ack_part= *this;
      return m;
    }

    Msg NackPartPayload::to_msg(agent_t to, agent_t from){
      Msg m;
      m.to=to;
      m.from=from;
      m.type=Msg::Type::NACK_PART;
      m.data.nack_part = *this;
      return m;
    }

    Msg JoinUsPayload::to_msg(agent_t to, agent_t from) {
      Msg m;
      m.to=to;
      m.from=from;
      m.type=Msg::Type::JOIN_US;
      m.data.join_us = *this;
      return m;
    }

    Msg::Msg(){
    }
    Msg::~Msg(){
    }

  }
}
