/**
 *   @copyright 
 *   Copyright (c) 2022 California Institute of Technology (“Caltech”). 
 *   U.S.  Government sponsorship acknowledged.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *    * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    *  Neither the name of Caltech nor its operating division, the Jet
 *    Propulsion Laboratory, nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file ghs-demo-comms.cpp
 *
 */
//and our header
#include "ghs-demo-comms.h"

#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/reqrep0/rep.h>
//#include <nng/protocol/pair0/pair.h>
#include <cassert>  //assert
#include <cstring>  //mem operations
#include <cstdio>   //printf and such
#include <unistd.h> //sleep
#include <mutex>
#include <chrono>

///
namespace demo{


  static Comms static_inst;

  Comms::Comms()
  {
    //TODO in an active system, some error recovery would load sequence from storage. 
    outgoing_seq= 1;
  }

  Comms::~Comms(){
    read_continues=false;
    nng_close(incoming);
    nng_close(outgoing);
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
  void Comms::validate_sockets(){

    //TODO set up the default options for send / recv on both incoming and
    //outgoing sockets to allow this single-threaded operation
    if (nng_socket_id(incoming) == -1){ 
      int ret = nng_rep0_open(&incoming);
      if (ret!=0){ assert(false); }
      //ret=(nng_socket_set_int(incoming, NNG_OPT_RECVBUF, 256));
      //ret=(nng_socket_set_size(incoming, NNG_OPT_RECVMAXSZ, sizeof(WireMessage)));
      ret=(nng_socket_set_ms(incoming, NNG_OPT_RECVTIMEO, nng_duration(5000)));
      assert(0==ret);
      //ret=(nng_socket_set_ms(incoming, NNG_OPT_SENDTIMEO, nng_duration(500)));
      //ret=(nng_socket_set_ms(incoming, NNG_OPT_RECONNMINT,nng_duration(100)));
      //ret=(nng_socket_set_ms(incoming, NNG_OPT_RECONNMAXT,nng_duration(10000)));
    }
    if (nng_socket_id(outgoing) == -1){ 
      int ret;
      ret=(nng_req0_open(&outgoing));
      if (ret!=0){ assert(false); }
      assert(0==ret);
      //ret=(nng_socket_set_ms(outgoing, NNG_OPT_RECVTIMEO, nng_duration(1000)));
      ret=(nng_socket_set_ms(outgoing, NNG_OPT_SENDTIMEO, nng_duration(1000)));
      assert(0==ret);
      //ret=(nng_socket_set_ms(outgoing, NNG_OPT_RECONNMINT,nng_duration(100)));
      //ret=(nng_socket_set_ms(outgoing, NNG_OPT_RECONNMAXT,nng_duration(10000)));
      //there's a couple of settings we need that are TCP specific, too...
      //NNG_OPT_TCP_KEEPALIVE  (true) to support "pings" periodically.
    }
  }

  Comms& Comms::with_config(Config &c)
  {
    validate_sockets();
    assert(cfg_is_ok(c));

    if (nng_listener_id(ghs_listener) != -1){
      int ret;
      ret=(nng_listener_close(ghs_listener));
      if (ret!=0){assert(false);}
    }

    printf("[info] Starting listener for ghs on : %s\n", c.endpoints[c.my_id]);
    int ret;
    ret=(nng_listener_create(&ghs_listener,incoming,c.endpoints[c.my_id]));
    if (ret!=0){assert(false);}
    assert(0==ret);
    ret=(nng_listener_start(ghs_listener,0));
    assert(0==ret);

    //std::copy you can go to hell
    memmove((void*)&ghs_cfg, &c, sizeof(Config));

    printf("[info] Initialized GHS comms subsystem!\n");

    return (*this);
  }

  //Comms& Comms::with_config(some_other_subsystem &s) { ... }

  bool Comms::ok()
  {
    //verify state
    return true;
  }

  int Comms::recv(WireMessage &buf)
  {

    WireMessage local_msg;
    //this line saves 100 lines of C++ in the definition of WireMessage (constructing
    //from buffer)
    auto local_msg_charptr =  (uint8_t*) &local_msg ;
    size_t recvsz=sizeof(WireMessage);

    printf("[info] RECV: waiting for msg ... \n");
    int ret = nng_recv( incoming, static_cast<void*>(local_msg_charptr), &recvsz, 0);
    //Note, local_msg is now populated

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

    /*
       if (debug){
       printf("[info] RECV: Read %zu :", recvsz);
       for (size_t i=0;i<recvsz;i++){
       printf("%x",local_msg_charptr[i]);
       }
       printf("\n");
       }*/

    printf("[info] RECV: sending confirmation\n");
    SequenceCounter msg_seq = local_msg.control.sequence;
    recvsz = sizeof(SequenceCounter);
    auto from = local_msg.header.agent_from;
    ret = nng_send(incoming,(void*)&msg_seq,recvsz,0);
    if (ret!=0){ 
      printf("[error] RECV: failure to send confirmation: %s\n", nng_strerror(ret));
    }

    //nng_sock assures delivery before returning, but may send multiple times.
    //So we use sequence #s to drop duplicates. But warn if it appears order is
    //violated, for debugging.

    if (msg_seq == 0) {
      printf("[warn] RECV: error on sending side, received seq==0 (payload not set?)\n");
      return 0;
    } else if (sequence_counters[from]>msg_seq){
      printf("[warn] RECV: msg seq indicates reorder!!, dropping and proceeding anyway\n");
      return 0;
    } else if (sequence_counters[from]==msg_seq){
      printf("[info] RECV: msg seq indicates duplicate, dropping\n");
      return 0;
    } else {
      sequence_counters[from]=msg_seq;
    }

    if (recvsz==0){
      printf("[error] RECV: no bytes read by nng, but message returned. Fatal.\n");
      return -1;
    } else {
      printf("[info] RECV: Copying to user buf\n");
      memmove(
          static_cast<void*>(&buf),
          static_cast<void*>(&local_msg),
          local_msg.size()
          );
    }

    return recvsz;
  }

  void Comms::read_loop()
  {
    while(read_continues)
    {

      WireMessage m;
      int ret = recv(m);

      if (ret<0){ //fatal
        read_continues = false;
      } else if (ret==0) { //nonfatal
        continue;
      } else { //take action by type
        switch (m.header.type){
          case PAYLOAD_TYPE_GHS:
            {
              std::lock_guard<std::mutex> guard(q_mut);
              dle::Errno ret;
              if ( (ret= in_q.push(m)) != dle::OK)
              {
                printf("[error] queue err: %s", dle::strerror(ret)); 
              }
              break;
            }
          case PAYLOAD_TYPE_METRICS:
            {
              auto from = m.header.agent_from;
              auto prior = kbps[from];
              auto sent  = *(Kbps*)&m.bytes[0];
              kbps[from] = std::min(prior, sent);;
              printf("[info] updated metrics to %u from %u b/c rec %u\n",kbps[from],prior, sent);
              break;
            }
          case PAYLOAD_TYPE_CONTROL: 
            {break;}
          case PAYLOAD_TYPE_PING: 
            {break;}
          default: 
            {
              printf("[error] unrecognized msg type: %d",m.header.type);
              break;
            }
        }
      }
      //while...
    }
  }

  bool    Comms::has_msg(){
    std::lock_guard<std::mutex> guard(q_mut);
    return in_q.size()>0;
  }

  bool Comms::get_next(WireMessage&m){
    std::lock_guard<std::mutex> guard(q_mut);
    if (in_q.size()>0){
      dle::Errno ret;
      if ( (ret=in_q.pop(m)) == dle::OK){
        return true;
      } else {
        switch(ret)
        {
          case dle::ERR_QUEUE_EMPTY:
            {
              printf("[error] Queue empty unexpectedly!");
              break;
            }
          case dle::ERR_BAD_IDX:
            {
              printf("[error] Queue error, no element 0!");
              break;
            }
          case dle::ERR_NO_SUCH_ELEMENT:
            {
              printf("[error] Queue error, no element 0!");
              break;
            }
          default:
            {
              printf("[error] unexpected queue return: %d", ret); 
              read_continues=false; 
              break;
            }
        }
      }
    }
    return false;
  }

  Errno Comms::send(WireMessage& msg, OptMask mask)
  {

    Destination destination = msg.header.agent_to;
    if (destination == MESSAGE_DEST_UNSET){
      return ERR_DEST_UNSET;
    }

    //infer endpoint from destination / subsystem pairs
    const char* endpoint; 
    switch (msg.header.type){
      case (PAYLOAD_TYPE_NOT_SET):{ return ERR_NO_PAYLOAD_TYPE ;}
      case (PAYLOAD_TYPE_GHS):{ endpoint = ghs_cfg.endpoints[destination]; break;}
      default: { return ERR_UNRECOGNIZED_PAYLOAD_TYPE; }
    }
    long us_rt;
    return internal_send(msg,endpoint,us_rt);
  }

  void Comms::exchange_iperf()
  {
    for (int i=0;i<ghs_cfg.n_agents;i++){
      if (i==ghs_cfg.my_id) continue;

      WireMessage m;
      m.header.type = PAYLOAD_TYPE_METRICS;
      m.header.agent_from = ghs_cfg.my_id;
      auto metric = kbps[i];
      printf("[info] sending: %u to %d\n",metric,i);
      auto msz    = sizeof(metric);
      m.header.payload_size=msz;
      memmove(m.bytes,&metric,msz);
      m.header.agent_to = i;
      const char* endpoint = ghs_cfg.endpoints[i];

      long us_rt;
      Errno ret;
      ret = internal_send(m,endpoint,us_rt);
      if (ret!=0){
        printf("Ignoring error: %d",ret);
      }
      //blithely ignore failures here
    }
  }

  void Comms::little_iperf(){
    //infer endpoint from destination / subsystem pairs
    size_t sz = std::min(PAYLOAD_MAX_SZ, 1024);
    WireMessage m;
    memset(m.bytes,0,sz);
    m.header.type = PAYLOAD_TYPE_PING;
    m.header.agent_from = ghs_cfg.my_id;
    m.header.payload_size=sz;
    for (int i=0;i<ghs_cfg.n_agents;i++){
      kbps[i]=0;
    }
    for (int repeat=0;repeat<10;repeat++){
      for (int i=0;i<ghs_cfg.n_agents;i++){

        if (i==ghs_cfg.my_id) continue;

        const char* endpoint = ghs_cfg.endpoints[i];
        m.header.agent_to = i;
        long us_rt;
        if (internal_send(m,endpoint,us_rt)==0){
          kbps[i]+=(2e6*m.size())/(us_rt*10);//2e5, really, but that's annoying
        }//if err: assume 0 kbps (adding 0 does nothing)
      }
    }
  }

  void Comms::print_iperf(){
    for (int i=0;i<ghs_cfg.n_agents;i++){
      printf("[info] avg kbps[%d]=%d\n",i,kbps[i]);
    }
  }

  Errno Comms::internal_send(WireMessage&m, const char* endpoint, long &us_rt)
  {
    std::lock_guard<std::mutex> guard(send_mut);
    m.control.sequence=outgoing_seq;
    void* outbuf = (void*)&m;
    size_t obsz = m.size();
    size_t return_seq=0;
    size_t return_seq_sz = sizeof(return_seq);
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;

    //Create connection
    nng_dialer dialer;
    printf("[info] Dialing: %s \n",endpoint);

    int ret = nng_dialer_create(&dialer, this->outgoing, endpoint);
    if (ret!=0){ 
      printf("[error] send() error: %s\n", nng_strerror(ret)); 
      return ERR_NNG;
    }

    //dialer exists...
    ret = nng_dialer_start(dialer,0);
    if (ret!=0){ goto send_cleanup; }

    start = std::chrono::steady_clock::now();
    ret = nng_send(this->outgoing,outbuf,obsz,0);
    if (ret!=0){ goto send_cleanup; } 
    if (obsz!=m.size()){
      printf("[error] sent: %zu/%zu\n",obsz,m.size());
    }

    ret = nng_recv( this->outgoing, (void*) &return_seq, &return_seq_sz, 0);
    if (ret!=0){ goto send_cleanup; } 
    printf("[info] Sent w/%zu, conf= %zu\n", m.control.sequence, return_seq);
    assert(return_seq == m.control.sequence);
    outgoing_seq++;

send_cleanup:

    if (ret!=0){ 
      printf("[error] send() error: %s\n", nng_strerror(ret)); 
      //TODO: handle ret, for example dialer error not avail means kbps=0.
    }
    else {
      //capture metrics
      end = std::chrono::steady_clock::now();
      auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end-start);
      us_rt = diff.count();
      printf("[info] Round-trip time: %ld \xC2\xB5s\n", us_rt);
    }

    int retclose=(nng_dialer_close(dialer));
    if (retclose!=0){
      printf("[error] closing dialer: %s",nng_strerror(retclose));
    }
    return ret==0?OK:ERR_NNG;
  }

  void Comms::start_receiver(){
    read_continues=true;
    reader_thread = std::thread(&Comms::read_loop, this);
  }

  void Comms::stop_receiver(){
    read_continues=false;
    reader_thread.join();
  }

  //Globally unique, symmetric metric.
  uint64_t Comms::unique_link_metric_to(const uint16_t agent_id) const
  {
    return sym_metric(agent_id, ghs_cfg.my_id, kbps[agent_id]);
  }


  Kbps Comms::kbps_to(const uint16_t to) const
  {
    return kbps[to];
  }
}
