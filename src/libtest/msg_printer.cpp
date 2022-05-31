#include "ghs/msg_printer.h"

#include <sstream>

using le::ghs::Msg;

std::string to_string(const Msg::Type &type){
	switch (type){
		case Msg::Type::NOOP:{return "NOOP";}
		case Msg::Type::SRCH:{return "SRCH";}
		case Msg::Type::SRCH_RET:{return "SRCH_RET";}
		case Msg::Type::IN_PART:{return "IN_PART";}
		case Msg::Type::ACK_PART:{return "ACK_PART";}
		case Msg::Type::NACK_PART:{return "NACK_PART";}
		case Msg::Type::JOIN_US:{return "JOIN_US";}
		default: {return "??";};
	}
}

std::ostream& operator << ( std::ostream& outs, const Msg::Type & type )
{
  return outs << to_string(type);
}


std::ostream& operator << ( std::ostream& outs, const Msg & m)
{
  using namespace le::ghs;
  outs << "("<<m.from<<"-->"<< m.to<<") "<< m.type<<" {";
  switch (m.type){
    case Msg::Type::NOOP:
      { break; }
    case Msg::Type::SRCH:
      {
        outs<<"ldr:"<<m.data.srch.your_leader<<" ";
        outs<<"lvl:"<<m.data.srch.your_level;
        break;
      }
    case Msg::Type::SRCH_RET:
      {
        outs<<"peer"<<m.data.srch_ret.to<<" ";
        outs<<"root:"<<m.data.srch_ret.from<<" ";
        outs<<"val:"<<m.data.srch_ret.metric;
        break;
      }
    case Msg::Type::IN_PART:
      {
        outs<<"ldr:"<<m.data.in_part.leader<<" ";
        outs<<"lvl:"<<m.data.in_part.level;
        break;
      }
    case Msg::Type::ACK_PART:
      { break; }
    case Msg::Type::NACK_PART:
      { break; }
    case Msg::Type::JOIN_US:
      {
        outs<<"peer:"<<m.data.join_us.join_peer<<" ";
        outs<<"root:"<<m.data.join_us.join_root<<" ";
        outs<<"root-ldr:"<<m.data.join_us.proposed_leader<<" ";
        outs<<"root-lvl:"<<m.data.join_us.proposed_level;
        break;
      }
  }
  outs<<"}";
  return outs;
}

