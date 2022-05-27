#include "ghs/msg_printer.hpp"

#include <sstream>

std::string to_string(const MsgType &type){
	switch (type){
		case MsgType::NOOP:{return "NOOP";}
		case MsgType::SRCH:{return "SRCH";}
		case MsgType::SRCH_RET:{return "SRCH_RET";}
		case MsgType::IN_PART:{return "IN_PART";}
		case MsgType::ACK_PART:{return "ACK_PART";}
		case MsgType::NACK_PART:{return "NACK_PART";}
		case MsgType::JOIN_US:{return "JOIN_US";}
		default: {return "??";};
	}
}

std::ostream& operator << ( std::ostream& outs, const MsgType & type )
{
  return outs << to_string(type);
}


std::ostream& operator << ( std::ostream& outs, const Msg & m)
{
  outs << "("<<m.from<<"-->"<< m.to<<") "<< m.type<<" {";
  switch (m.type){
    case NOOP:
      { break; }
    case SRCH:
      {
        outs<<"ldr:"<<m.data.srch.your_leader<<" ";
        outs<<"lvl:"<<m.data.srch.your_level;
        break;
      }
    case SRCH_RET:
      {
        outs<<"peer"<<m.data.srch_ret.to<<" ";
        outs<<"root:"<<m.data.srch_ret.from<<" ";
        outs<<"val:"<<m.data.srch_ret.metric;
        break;
      }
    case IN_PART:
      {
        outs<<"ldr:"<<m.data.in_part.leader<<" ";
        outs<<"lvl:"<<m.data.in_part.level;
        break;
      }
    case ACK_PART:
      { break; }
    case NACK_PART:
      { break; }
    case JOIN_US:
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

