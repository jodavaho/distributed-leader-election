#include "msg.hpp"

std::string to_string(const Msg::Type &type){
	switch (type){
		case Msg::Type::SRCH:{return "SRCH";}
		case Msg::Type::SRCH_RET:{return "SRCH_RET";}
		case Msg::Type::IN_PART:{return "IN_PART";}
		case Msg::Type::ACK_PART:{return "ACK_PART";}
		case Msg::Type::NACK_PART:{return "NACK_PART";}
		case Msg::Type::JOIN_US:{return "JOIN_US";}
		case Msg::Type::JOIN_OK:{return "JOIN_OK";}
		case Msg::Type::ELECTION:{return "ELECTION";}
		case Msg::Type::NOT_IT:{return "NOT_IT";}
		case Msg::Type::NEW_SHERIFF:{return "NEW_SHERIFF";}
		default: {return "??";};
	}
}

std::ostream& operator << ( std::ostream& outs, const Msg::Type & type )
{
  return outs << to_string(type);
}


std::ostream& operator << ( std::ostream& outs, const Msg & m)
{
  outs << "("<<m.from<<"-->"<< m.to<<") "<< m.type<<" {";
  for (const auto data: m.data){
	  outs<<" "<<data;
  }
  outs<<" }";
  return outs;
}

