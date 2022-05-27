#ifndef GHS_DEMO_COMMS
#define GHS_DEMO_COMMS 
#include "ghs-demo-config.h"
#include "ghs-demo-msgutils.hpp"
#include "ghs-demo-edgemetrics.hpp"
#include "seque/seque.hpp"
#include <nng/nng.h>//req_s, rep_s, msg
#include <cstring> //memcpy, memset
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>

#ifndef COMMS_DEMO_MAX_N
#define COMMS_DEMO_MAX_N 8
#endif

#ifndef PAYLOAD_MAX_SZ
#define PAYLOAD_MAX_SZ 1024
#endif

typedef uint8_t MessageErrno;
#define MessageOK           0
#define ERR_BAD_PAYLOAD_SZ  1
#define ERR_NO_PAYLOAD_TYPE 2
#define ERR_UNRECOGNIZED_PAYLOAD_TYPE 3
#define ERR_NULL_SRC        4
#define ERR_DEST_UNSET      5
#define ERR_HANGUP         16

typedef uint8_t PayloadType;
#define PAYLOAD_TYPE_NOT_SET 0
#define PAYLOAD_TYPE_CONTROL 1
#define PAYLOAD_TYPE_METRICS 2
#define PAYLOAD_TYPE_PING    3
#define PAYLOAD_TYPE_GHS     4

typedef uint8_t ControlCommand;
#define CONTROL_TEST_MSG      1
#define CONTROL_LATENCY       2
#define CONTROL_START_GHS     12
#define CONTROL_RESET_GHS     13

typedef uint64_t MessageOptMask;

typedef uint16_t MessageDest;
//stay with me, twos complement
#define MESSAGE_DEST_UNSET -1


typedef size_t SequenceCounter;

struct 
__attribute__((__packed__,aligned(1)))
MessageHeader
{
  MessageDest agent_to=MESSAGE_DEST_UNSET;
  uint16_t agent_from;
  PayloadType type=PAYLOAD_TYPE_NOT_SET; 
  uint16_t payload_size=0;
};

struct 
__attribute__((__packed__,aligned(1)))
MessageControl
{
  SequenceCounter sequence=0;
};

struct 
__attribute__((__packed__,aligned(1)))
Message
{
  MessageControl control;
  MessageHeader header;
  uint8_t bytes[PAYLOAD_MAX_SZ];
  size_t bus_size()  const
  { 
    return sizeof(MessageHeader)
      +sizeof(MessageControl)
      +header.payload_size; 
  }
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

    //implementation specific, I use plain old threads here.  Note, ZMQ does
    //this for you, but requires a lot of std:: and platform assumptions.
    //So, we avoid *some of* that with nng, and one use of std::thread.
    void start_receiver();
    void stop_receiver();

    bool has_msg();
    bool get_next(Message&);

    void little_iperf();
    void exchange_iperf();
    void print_iperf();

    Kbps kbps_to(const uint16_t agent_id) const;
    CommsEdgeMetric unique_link_metric_to(const uint16_t agent_id) const;
   
  private:

    void validate_sockets();
    void read_loop();
    int recv(Message& buf);
    MessageErrno internal_send(Message&m, const char* endpoint, long &us_rt);

    StaticQueue<Message,1024> in_q;
    bool read_continues;
    ghs_config ghs_cfg;
    nng_listener ctr_listener = NNG_LISTENER_INITIALIZER;
    nng_listener ghs_listener = NNG_LISTENER_INITIALIZER;
    nng_socket incoming=NNG_SOCKET_INITIALIZER;
    nng_socket outgoing=NNG_SOCKET_INITIALIZER;
    size_t outgoing_seq;
    std::array<size_t,COMMS_DEMO_MAX_N> sequence_counters;
    std::array<Kbps,COMMS_DEMO_MAX_N> kbps;
    std::mutex q_mut;
    std::mutex send_mut;
    std::thread reader_thread;

};



#endif
