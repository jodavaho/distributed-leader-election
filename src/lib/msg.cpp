#include "ghs/msg.hpp"

Msg SrchPayload::to_msg(AgentID to, AgentID from){
  Msg m;
  m.to=to;
  m.from=from;
  m.type=SRCH;
  m.data.srch = *this;
  return m;
}

Msg SrchRetPayload::to_msg(AgentID to, AgentID from){
  Msg m;
  m.to=to;
  m.from=from;
  m.type=SRCH_RET;
  m.data.srch_ret = *this;
  return m;
}


Msg InPartPayload::to_msg(AgentID to, AgentID from){
  Msg m;
  m.to=to;
  m.from=from;
  m.type=IN_PART;
  m.data.in_part= *this;
  return m;
}

Msg AckPartPayload::to_msg(AgentID to, AgentID from){
  Msg m;
  m.to=to;
  m.from=from;
  m.type=ACK_PART;
  m.data.ack_part= *this;
  return m;
}

Msg NackPartPayload::to_msg(AgentID to, AgentID from){
  Msg m;
  m.to=to;
  m.from=from;
  m.type=NACK_PART;
  m.data.nack_part = *this;
  return m;
}

Msg JoinUsPayload::to_msg(AgentID to, AgentID from) {
  Msg m;
  m.to=to;
  m.from=from;
  m.type=JOIN_US;
  m.data.join_us = *this;
  return m;
}


/*
Msg::Msg (const MsgType &type,
      const AgentID &send_to,
      const AgentID &send_from,
      const MsgData &payload){
  this->type=type;
  this->to = send_to;
  this->from=send_from;
  switch (type){
    {
      case NOOP:{data.noop=payload;break;}
      case SRCH:{data.srch=payload;break;}
      case SRCH_RET:{data.srch_ret=payload;break;}
      case IN_PART:{data.in_part=payload;break;}
      case ACK_PART:{data.ack_part=payload;break;}
      case NACK_PART:{data.nack_part=payload;break;}
      case JOIN_US:{data.join_us=payload;break;}
      default:{break;}
  }
}
*/

Msg::Msg(){
}
Msg::~Msg(){
}

