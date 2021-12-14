#ifndef msg_hpp
#define msg_hpp

#include <iostream>
#include <vector>

typedef struct __attribute__((__packed__))
{
  int8_t to;
  int8_t from;
  int8_t data_byte;
  int8_t crc8;
} wire_msg; //example only, need 1 per type below

typedef struct
{
	enum Type
	{
		NOOP = 0,
		SRCH,
		SRCH_RET, 
		IN_PART,  
		ACK_PART,
		NACK_PART,
		JOIN_US,
    JOIN_OK,
		ELECTION,
		NOT_IT,
		NEW_SHERIFF
	} type;
	size_t to, from;
	std::vector<size_t> data;
} Msg;

wire_msg to_wire(const Msg&m);
Msg to_msg (const wire_msg&m);

std::string to_string(const Msg::Type &type);

std::ostream& operator << ( std::ostream& outs, const Msg::Type & type );

std::ostream& operator << ( std::ostream& outs, const Msg & m);

#endif
