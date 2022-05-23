#ifndef GHS_DEMO_COMMS
#define GHS_DEMO_COMMS 
#include "ghs-demo-inireader.h"
#include "ghs-demo-msgutils.hpp"
#include "seque/seque.hpp"
#include <nng/nng.h>//req_s, rep_s, msg
#include <cstring> //memcpy, memset
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>

#ifndef PAYLOAD_MAX_SZ
//only information we have is:
#include "ghs/msg.hpp"
#define PAYLOAD_MAX_SZ GHS_MAX_MSG_SZ
#endif

typedef uint8_t MessageErrno;
#define ERR_BAD_PAYLOAD_SZ  1
#define ERR_NO_PAYLOAD_TYPE 2
#define ERR_UNRECOGNIZED_PAYLOAD_TYPE 3
#define ERR_NULL_SRC        4
#define ERR_DEST_UNSET      5
#define ERR_HANGUP         16

typedef uint8_t PayloadType;
#define PAYLOAD_TYPE_NOT_SET 0
#define PAYLOAD_TYPE_CONTROL 1
#define PAYLOAD_TYPE_GHS     2
#define PAYLOAD_TYPE_SYSTEM  255

typedef uint8_t ControlCommand;
#define CONTROL_TEST_MSG      1
#define CONTROL_START_GHS     12
#define CONTROL_RESET_GHS     13

typedef uint64_t MessageOptMask;
#define MESSAGE_OPT_NOBLOCK   1

typedef uint8_t MessageDest;
#define MESSAGE_DEST_CONVERGE   253
#define MESSAGE_DEST_BROADCAST  254
#define MESSAGE_DEST_UNSET      255

struct 
__attribute__((__packed__))
PayloadBytes
{
  PayloadType type=PAYLOAD_TYPE_NOT_SET; 
  uint8_t size=0;
  uint8_t bytes[PAYLOAD_MAX_SZ]={0}; 
};

struct 
__attribute__((__packed__,aligned(1)))
Message
{

  MessageDest agent_to=MESSAGE_DEST_UNSET;
  uint8_t agent_from;
  PayloadBytes payload;
  size_t sequence=0;

};

class DemoComms
{
  public:
    DemoComms();
    ~DemoComms();
    bool ok();

    //We should keep one on hand.
    static DemoComms& inst();
    
    //Initialize all data structures for the given node set / endpoints
    DemoComms& with_config(ghs_config &);

    //e.g. for another subsystem
    //DemoComms& with_config(database_config &);
    
    //the "full functionality". Type determines config to use
    MessageErrno send(Message&, MessageOptMask=0);
    
    //shortcuts to use MST-based multi-cast (recommended by radio datasheets)
    MessageErrno broadcast_bytes(const PayloadBytes&);
    MessageErrno convergecast_message(const PayloadBytes&);

    //implementation specific, I use plain old threads here.  Note, ZMQ does
    //this for you, but requires a lot of std:: and platform assumptions.
    //So, we avoid that with nng, and one use of std::thread.
    //
    //I also dont use boost lock-free data structures, which is more about
    //code size / compile time than anything else. 
    void start_receiver();
    void stop_receiver();
    
    //fun with networks!
    bool reachable_directly(uint8_t node);
    bool reachable_at_all(uint8_t node);

    bool has_msg();
    bool get_next(Message&m);


  private:
    nng_socket outgoing=NNG_SOCKET_INITIALIZER;
    nng_socket incoming=NNG_SOCKET_INITIALIZER;

    //one for each subsystem using this socket. You can have N listeners for
    //one incomgin socket.
    nng_listener ghs_listener = NNG_LISTENER_INITIALIZER;
    //e.g. database_listener = NNG_LISTENER_INITIALIZER;

    //Until I learn othewise, I'm using stack-stored dialers for each outgoing
    //msg queue.

    //the options for each subsystem
    ghs_config ghs_cfg;
    //e.g. database_config db_cfg;

    void validate_sockets();

    //this should be a static data structure
    std::unordered_map<size_t,size_t> sequence_counters;
    size_t outgoing_seq;

    void read_loop();
    bool read_continues;

    StaticQueue<Message,1024> in_q;
    std::mutex q_mut;
    std::thread reader_thread;
    int recv(Message& buf);

};



#endif
