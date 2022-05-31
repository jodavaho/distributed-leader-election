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
