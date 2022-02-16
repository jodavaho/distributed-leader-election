#ifndef msg_hpp
#define msg_hpp

#include "graph.hpp"


/*
typedef struct __attribute__((__packed__))
{
  int8_t to;
  int8_t from;
  int8_t data_byte;
  int8_t crc8;
} wire_msg; //example only, need 1 per type below
*/

struct Msg;

struct NoopPayload{
};

struct SrchPayload{
  AgentID your_leader;
  Level   your_level;
  Msg to_msg(AgentID to, AgentID from);
};

struct SrchRetPayload{
  AgentID to;
  AgentID from;
  size_t metric;
  Msg to_msg(AgentID to, AgentID from);
};

struct InPartPayload{
  AgentID leader;
  Level   level; 
  Msg to_msg(AgentID to, AgentID from);
};

struct AckPartPayload{ 
  Msg to_msg(AgentID to, AgentID from);
};

struct NackPartPayload{ 
  Msg to_msg(AgentID to, AgentID from);
};

struct JoinUsPayload{ 
  AgentID join_peer;
  AgentID join_root;
  AgentID proposed_leader;
  Level proposed_level;
  Msg to_msg(AgentID to, AgentID from) const;
};

enum MsgType
{
  NOOP = 0,
  SRCH,
  SRCH_RET, 
  IN_PART,  
  ACK_PART,
  NACK_PART,
  JOIN_US,
};

//desire: std::variant
union MsgData{
  NoopPayload noop;
  SrchPayload srch;
  SrchRetPayload srch_ret;
  InPartPayload in_part;
  AckPartPayload ack_part;
  NackPartPayload nack_part;
  JoinUsPayload join_us;
};


struct Msg
{

  Msg();
  ~Msg();
  MsgType type;
  AgentID to, from;
  MsgData data;

};

#endif
