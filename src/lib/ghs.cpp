#include "ghs/ghs.hpp"

bool GhsOK(const GhsError &e){ return e>=0; }

const char* ghs_strerror( const GhsError & e){
  switch (e){
    case GHS_OK:{return "No Error";}
    case GHS_PROCESS_SELFMSG:{return "Cannot process msg from self";}
    case GHS_PROCESS_NOTME:{return "Cannot process msg to a diff. agent!";}
    case GHS_PROCESS_INVALID_TYPE:{return "Do not know how to process this type of msg";}
    case GHS_PROCESS_REQ_MST:{return "This msg did not come from MST link, as it should have";}
    case GHS_SRCH_INAVLID_SENDER:{return "Cannot process SRCH msg from this agent. Need valid tree edge (or self-sent)";}
    case GHS_SRCH_STILL_WAITING:{return "Received srch msg from parent, while still waiting for replies -- new round?";}
    case GHS_ERR_QUEUE_MSGS:{return "The outgoing queue did not capture all our msgs";}
    case GHS_PROCESS_NO_EDGE_FOUND:{return "do not have an edge to the sender!";}
    case GHS_UNEXPECTED_SRCH_RET:{return "received a srch_ret call, but were not expecting it";}
    case GHS_ACK_NOT_WAITING:{return "received ACK from an agent we were not waiting for";}
    case GHS_BAD_MSG:{return "A malformed msg was received";}
    case GHS_JOIN_BAD_LEADER:{return "Not on parition boundary, and received join_us from unrecognized leader";}
    case GHS_JOIN_BAD_LEVEL:{return "Not on partition boundary, and rec'd join_us with a different level";}
    case GHS_JOIN_INIT_BAD_LEADER:{return "Unrecognized leader id when initiating join with another partition";}
    case GHS_JOIN_INIT_BAD_LEVEL:{return "Different/incorrect level when initiating join with another partition";}
    case GHS_JOIN_MY_LEADER:{return "other partition suggested I join my partition";}
    case GHS_JOIN_UNEXPECTED_REPLY:{return "rec higher-level join_us: impossible since we should not have replied to their in_part yet";}
    case GHS_ERR_IMPL: {return "Developer error: Reached 'unreachable' branch";}
    case GHS_CAST_INVALID_EDGE:{return "*cast operation failed with bad outgoing edge";}
    case GHS_SET_INVALID_EDGE:{return "set/add edge operation failed b/c of bad edge";}
    case GHS_PARENT_UNRECOGNIZED:{return "cannot set parent to unrecognized node (no edge)";}
    case GHS_PARENT_REQ_MST:{return "cannot set parent to non-MST node (bad edge type)";}
    case GHS_NO_SUCH_PEER:{return "Cannot find peer idx -- no edge or unrecognized id?";}
    case GHS_IMPL_REQ_PEER_MY_ID:{return "Cannot treat my_id as peer -- likely implementation error or malformed msg";}
    case GHS_TOO_MANY_AGENTS:{return "set/add edge operation failed b/c too many agents";}

    default: {return "Developer error: Unknown Error Code";}
  }
}

