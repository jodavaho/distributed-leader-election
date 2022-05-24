//and our header
#include "ghs-demo-comms.hpp"

#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/reqrep0/rep.h>
//#include <nng/protocol/pair0/pair.h>
#include <cassert>  //assert
#include <cstring>  //mem operations
#include <cstdio>   //printf and such
#include <unistd.h> //sleep

static DemoComms static_inst;

DemoComms::DemoComms()
{
  //TODO in an active system, some error recovery would load sequence from storage. 
  outgoing_seq= 1;
}

DemoComms::~DemoComms(){
  read_continues=false;
  nng_close(incoming);
  nng_close(outgoing);
}

DemoComms& DemoComms::inst()
{
  return static_inst;
}

/** 
 * Q: WHY REQ/REP SOCKETS?
 *
 * A: Look, we could use survey/respondent sockets for a lot of the GHS
 * algorithm, but at this time, due to the radio, it's not clear if that's
 * the right approach. In fact the radio specifically says we should *not*
 * broadcast, and that (being a mesh) msgs to agent A may go through B.  For
 * that reason, I _recommend_ that any broadcast-like action be handled with
 * an MST-based broadcast to avoid duplication. See ghs.hpp, which could very
 * easily be ported to the comms handler. 
 *
 * So, that leaves pt to pt comms, with fanout handled in the application
 * layer.  That means nng_pair (1-1) or nng_req/nng_rep. The docs for req/rep
 * say that it is reliable, because it will retry until a reply is  received.
 * This reply is not handled at lower levels. it literally means that the
 * application code has to package and send a response packet like an RPC call.
 * You can't, for example, queue up reqs and handle them one at a time, because
 * those req-holders will be waiting for replies (i.e., blocked). They can go
 * on and send more reqs and override the last one, but now we're just back to
 * nng_pair conops.
 *
 * The docs for pair say:
 *
 * > Even though this mode may appear to be "reliable", because back-pressure
 * > prevents discarding messages most of the time there are topologies
 * > involving devices (see nng_device(3)) or raw mode sockets where messages 
 * > may be discarded
 *
 * If we do go with pair, I hope (https://stackoverflow.com/q/72293870/389599)
 * that this means the network stack will handle reliable delivery at lvl 3,
 * rather than trying to finagle something at lvl 4+. 
 *
 */
void DemoComms::validate_sockets(){

  //TODO set up the default options for send / recv on both incoming and
  //outgoing sockets to allow this single-threaded operation
  if (nng_socket_id(incoming) == -1){ 
    assert(0== nng_rep0_open(&incoming));
    //assert(0==nng_socket_set_int(incoming, NNG_OPT_RECVBUF, 256));
    //assert(0==nng_socket_set_size(incoming, NNG_OPT_RECVMAXSZ, sizeof(Message)));
    assert(0==nng_socket_set_ms(incoming, NNG_OPT_RECVTIMEO, nng_duration(5000)));
    //assert(0==nng_socket_set_ms(incoming, NNG_OPT_SENDTIMEO, nng_duration(500)));
    //assert(0==nng_socket_set_ms(incoming, NNG_OPT_RECONNMINT,nng_duration(100)));
    //assert(0==nng_socket_set_ms(incoming, NNG_OPT_RECONNMAXT,nng_duration(10000)));
  }
  if (nng_socket_id(outgoing) == -1){ 
    assert(0==nng_req0_open(&outgoing));
    //assert(0==nng_socket_set_ms(outgoing, NNG_OPT_RECVTIMEO, nng_duration(1000)));
    assert(0==nng_socket_set_ms(outgoing, NNG_OPT_SENDTIMEO, nng_duration(1000)));
    //assert(0==nng_socket_set_ms(outgoing, NNG_OPT_RECONNMINT,nng_duration(100)));
    //assert(0==nng_socket_set_ms(outgoing, NNG_OPT_RECONNMAXT,nng_duration(10000)));
    //there's a couple of settings we need that are TCP specific, too...
    //NNG_OPT_TCP_KEEPALIVE  (true) to support "pings" periodically.
  }
}

DemoComms& DemoComms::with_config(ghs_config &c)
{
  validate_sockets();
  assert(ghs_cfg_is_ok(c));

  if (nng_listener_id(ghs_listener) != -1){
    assert(0==nng_listener_close(ghs_listener));
  }

  printf("[info] Starting listener for ghs on : %s\n", c.endpoints[c.my_id]);
  assert(0==nng_listener_create(&ghs_listener,incoming,c.endpoints[c.my_id]));
  assert(0==nng_listener_start(ghs_listener,0));

  //std::copy you can go to hell
  memmove((void*)&ghs_cfg, &c, sizeof(ghs_config));

  printf("[info] Initialized GHS comms subsystem!\n");

  return (*this);
}

//DemoComms& DemoComms::with_config(some_other_subsystem &s) { ... }

bool DemoComms::ok()
{
  //verify state
  return true;
}

int DemoComms::recv(Message &buf)
{

  Message local_msg;
  //this line saves 100 lines of C++ in the definition of Message?
  auto local_buf =  (uint8_t*) &local_msg ;
  size_t recvsz=sizeof(Message);

  printf("[info] RECV: waiting for msg ... \n");
  int ret = nng_recv( incoming, static_cast<void*>(local_buf), &recvsz, 0);
  const char* errstr = nng_strerror(ret);
  printf("[info] RECV: %s\n",errstr);

  switch (ret){
    case 0: {break;}
    case NNG_EAGAIN: {return 0;}
    case NNG_ETIMEDOUT : {return 0;}
    case NNG_ECLOSED: {printf("[error] RECV: Socket closed! Fatal.\n");return -ret;}
    case NNG_EINVAL: {printf("[error] RECV: invalid flags! Fatal programmer error.\n");return -ret;}
    case NNG_EMSGSIZE: {printf("[error] RECV: invalid msg size -- too big! nonfatal\n");return 0;}
    case NNG_ENOMEM: {printf("[error] RECV: Out of memory. fatal!\n");return -ret;}
    case NNG_ENOTSUP: {printf("[error] RECV: cannot recv on this socket. fatal programmer error!\n");return -ret;}
    case NNG_ESTATE: {printf("[error] RECV: cannot recv on this socket at this time -- did you use the right protocol and correct socket creation function? fatal programmer error!\n");return -ret;}
    default: {printf("[error] RECV: Unknown error! %d:%s", ret, errstr);return -ret;}
  }

  printf("[info] RECV: Read %zu :", recvsz);
  for (size_t i=0;i<sizeof(Message);i++){
    printf("%x",local_buf[i]);
  }
  printf("\n");

  printf("[info] RECV: sending confirmation\n");
  recvsz = sizeof(size_t);
  ret = nng_send(incoming,(void*)&local_msg.sequence,recvsz,0);
  if (ret!=0){ 
    printf("[error] RECV: failure to send confirmation: %s\n", nng_strerror(ret));
  }

  //nng_sock assures delivery before returning, but may send multiple times.
  //So we use sequence #s to drop duplicates. But warn if it appears order is
  //violated, for debugging.
  if (local_msg.sequence == 0) {
    printf("[warn] RECV: error on sending side, received seq==0\n");
    return 0;
  } else if (sequence_counters[local_msg.agent_from]>local_msg.sequence){
    printf("[warn] RECV: msg seq indicates reorder!!, dropping and proceeding anyway\n");
    return 0;
  } else if (sequence_counters[local_msg.agent_from]==local_msg.sequence){
    printf("[info] RECV: msg seq indicates duplicate, dropping\n");
    return 0;
  } else {
    sequence_counters[local_msg.agent_from]=local_msg.sequence;
  }

  if (recvsz==0){
    printf("[error] RECV: no bytes read by nng, but message returned. Fatal.\n");
    return -1;
  } else {
    printf("[info] RECV: Decompressing\n");

    printf("[info] RECV: Copying to user buf\n");
    memmove(
        static_cast<void*>(&buf),
        static_cast<void*>(&local_msg),
        sizeof(Message)
        );
  }

  return recvsz;
}

MessageErrno DemoComms::send(Message& msg, MessageOptMask mask)
{

  size_t recvsz = sizeof(Message);
  size_t ret_seq = 0;

  MessageDest destination = msg.agent_to;
  if (destination == MESSAGE_DEST_UNSET){
    return ERR_DEST_UNSET;
  }

  //infer endpoint from destination / subsystem pairs
  const char* endpoint; 
  switch (msg.payload.type){
    case (PAYLOAD_TYPE_NOT_SET):{ return ERR_NO_PAYLOAD_TYPE ;}
    case (PAYLOAD_TYPE_CONTROL):{ endpoint = ghs_cfg.endpoints[destination]; break;}
    case (PAYLOAD_TYPE_GHS):{ endpoint = ghs_cfg.endpoints[destination]; break;}
    default: { return ERR_UNRECOGNIZED_PAYLOAD_TYPE; }
  }

  //// Msg is OK, endpoint is ok
  msg.sequence = outgoing_seq;

  //Create connection
  nng_dialer dialer;
  printf("[info] Dialing: %s \n",endpoint);

  int ret = nng_dialer_create(&dialer, outgoing, endpoint);
  if (ret!=0){ 
    printf("[error] send() error: %s\n", nng_strerror(ret)); 
    return ret;
  }

  //dialer exists...
  ret = nng_dialer_start(dialer,0);
  if (ret!=0){ goto send_cleanup; }

  ret = nng_send(outgoing,(void*)&msg,sizeof(Message),0);
  if (ret!=0){ goto send_cleanup; } 

  recvsz = sizeof(ret_seq);
  ret = nng_recv( outgoing, (void*) &ret_seq, &recvsz, 0);
  if (ret!=0){ goto send_cleanup; } 
  printf("[info] Sent # %zu, conf= %zu\n", ret_seq, msg.sequence);
  assert(ret_seq == msg.sequence);
  outgoing_seq++;

send_cleanup:

  if (ret!=0){ printf("[error] send() error: %s\n", nng_strerror(ret)); }
  assert(0==nng_dialer_close(dialer));

  return ret; 
}

void DemoComms::read_loop()
{
  while(read_continues)
  {

    Message m;
    int ret = recv(m);
    if (ret<0){
      //fatal
      read_continues = false;
    } else if (ret==0) {
      //nonfatal
      continue;
    } else {
      //push
      std::lock_guard<std::mutex> guard(q_mut);
      QueueRetcode ret;
      if ( (ret= in_q.push(m)) != OK)
      {
        switch (ret)
        {
          case ERR_QUEUE_FULL: { 
                                 printf("[error] Queue full!"); 
                                 read_continues=false; 
                                 break; 
                               }
          default: {
                     printf("[error] unexpected queue return: %d", ret); 
                     read_continues=false; 
                     break;}
        }
      }
    }
  }
}

bool    DemoComms::has_msg(){
  std::lock_guard<std::mutex> guard(q_mut);
  return in_q.size()>0;
}

bool DemoComms::get_next(Message&m){
  std::lock_guard<std::mutex> guard(q_mut);
  if (in_q.size()>0){
    QueueRetcode ret;
    if ( (ret=in_q.pop(m)) == OK){
      return true;
    } else {
      switch(ret)
      {
        case ERR_QUEUE_EMPTY:{
                               printf("[error] Queue empty unexpectedly!");
                               break;
                             }
        case ERR_BAD_IDX:{
                           printf("[error] Queue error, no element 0!");
                           break;
                         }
        case ERR_NO_SUCH_ELEMENT:{
                                   printf("[error] Queue error, no element 0!");
                                   break;
                                 }
        default:{
                  printf("[error] unexpected queue return: %d", ret); 
                  read_continues=false; 
                  break;
                }
      }
    }
  }
  return false;
}

void DemoComms::start_receiver(){
  read_continues=true;
  reader_thread = std::thread(&DemoComms::read_loop, this);
}

void DemoComms::stop_receiver(){
  read_continues=false;
  reader_thread.join();
}
