#ifndef msg_hpp
#define msg_hpp

#include "graph.hpp"
#include <cstdint>
#include <cstring>

struct Msg;//the actual structure used internally


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

const size_t GHS_MAX_MSG_SZ= sizeof(Msg);


#endif
