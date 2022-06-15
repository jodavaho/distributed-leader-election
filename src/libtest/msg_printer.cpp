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
 * @file msg_printer.cpp
 *
 */
#include "ghs/msg_printer.h"

#include <sstream>

using le::ghs::Msg;
using namespace le::ghs;

std::string to_string(const msg::Type &type){
	switch (type){
		case msg::Type::NOOP:{return "NOOP";}
		case msg::Type::SRCH:{return "SRCH";}
		case msg::Type::SRCH_RET:{return "SRCH_RET";}
		case msg::Type::IN_PART:{return "IN_PART";}
		case msg::Type::ACK_PART:{return "ACK_PART";}
		case msg::Type::NACK_PART:{return "NACK_PART";}
		case msg::Type::JOIN_US:{return "JOIN_US";}
		default: {return "??";};
	}
}

std::ostream& operator << ( std::ostream& outs, const msg::Type & type )
{
  return outs << to_string(type);
}


std::ostream& operator << ( std::ostream& outs, const Msg & m)
{
  using namespace le::ghs;
  outs << "("<<m.from()<<"-->"<< m.to()<<") "<< m.type()<<" {";
  switch (m.type()){
    case msg::Type::UNASSIGNED: { break; }
    case msg::Type::NOOP:
      { break; }
    case msg::Type::SRCH:
      {
        outs<<"ldr:"<<m.data().srch.your_leader<<" ";
        outs<<"lvl:"<<m.data().srch.your_level;
        break;
      }
    case msg::Type::SRCH_RET:
      {
        outs<<"peer"<<m.data().srch_ret.to<<" ";
        outs<<"root:"<<m.data().srch_ret.from<<" ";
        outs<<"val:"<<m.data().srch_ret.metric;
        break;
      }
    case msg::Type::IN_PART:
      {
        outs<<"ldr:"<<m.data().in_part.leader<<" ";
        outs<<"lvl:"<<m.data().in_part.level;
        break;
      }
    case msg::Type::ACK_PART:
      { break; }
    case msg::Type::NACK_PART:
      { break; }
    case msg::Type::JOIN_US:
      {
        outs<<"peer:"<<m.data().join_us.join_peer<<" ";
        outs<<"root:"<<m.data().join_us.join_root<<" ";
        outs<<"root-ldr:"<<m.data().join_us.proposed_leader<<" ";
        outs<<"root-lvl:"<<m.data().join_us.proposed_level;
        break;
      }
  }
  outs<<"}";
  return outs;
}

