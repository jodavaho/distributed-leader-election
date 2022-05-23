
#include <signal.h>
#include <unistd.h> //sleep
#include <sstream> //better than iostream

#include "ghs-demo-inireader.h"
#include "ghs-demo-msgutils.hpp"
#include "ghs-demo-comms.hpp"
#include "ghs/ghs_printer.hpp" //dump_edges

#include "ghs/ghs.hpp"
#include "ghs/msg_printer.hpp" //for printing GHS msgs.
#include "ghs/graph.hpp"
#include "seque/seque.hpp"

template<size_t AN, size_t QN>
void initialize_ghs(GhsState<AN,QN>& ghs, ghs_config& cfg)
{
  ghs.reset();
  for (AgentID i=0;i<cfg.n_agents;i++){
    if (i!=cfg.my_id){
      ghs.set_edge( {i,cfg.my_id,UNKNOWN, i+cfg.my_id} ); 
      printf("[info] Set edge: %d from %d (check=%d)\n",
          i,
          cfg.my_id,
          ghs.has_edge(i));
      Edge e;
      ghs.get_edge(i,e);
      printf("[info] (%d<--%d, %d %d)\n",
          e.peer,e.root,e.status,e.metric_val);
    }
  }
}

template<size_t AN, size_t QN>
void kill_edge(GhsState<AN,QN>& ghs, ghs_config & cfg, uint8_t agent_to)
{
  bool resp_req=false, waiting_for=false;
  ghs.set_edge( {agent_to, cfg.my_id, DELETED, 0} );

  ghs.is_response_required(agent_to, resp_req);
  if (resp_req)
  {
    printf("[info] response impossible: alerting GHS state machine to loss of agent\n");
    ghs.set_response_required(agent_to, false);
  }

  ghs.is_waiting_for(agent_to, waiting_for);
  if (waiting_for){
    printf("[info] waiting illogical: alerting GHS state machine to loss of agent\n");
    ghs.set_waiting_for(agent_to, false);
  }
}

int do_test_and_die(DemoComms& comms, ghs_config &config){
  printf("Waiting 10 s and then dying\n");
  comms.start_receiver();
  if (config.my_id==0){
    printf("Sending to %d agents ... \n ", config.n_agents);
    for (int s =1;s<=10;s++){
      sleep(1);
      for (int i=1;i<config.n_agents;i++){
        printf("%d ... \n ", i);
        Message m;
        m.agent_to=i;
        m.agent_from=config.my_id;
        m.payload.type=PAYLOAD_TYPE_CONTROL;
        m.sequence=s;
        comms.send(m);
      }
    }
  } else {
    for (int s =0;s<10;s++){
      sleep (1);
      if (comms.has_msg()){
        Message m;
        comms.get_next(m);
        printf("Received from%d seq=%zu!\n",m.agent_from,m.sequence);
      } else {
        printf("%d\n",s);
      }
    }
  }
  printf("test done!\n");
  comms.stop_receiver();
  return 0;
}

//each node will have a listener / rep, where it can receive msgs. 
//each node will also set up a req/dialer to send msgs to other nodes. 
int main(int argc, char** argv){

  ghs_config config;

  //can we read configs without allocating memory!?
  //Yes we can!
  ghs_read_cfg_stdin(&config);
  ghs_read_cfg_cli(argc,argv,&config);

  printf("[info] Done configuring for id=%d... \n",config.my_id);

  if (!ghs_cfg_is_ok(config)){
    return 1;
  }

  printf("[info] Sizeof(Message)=%zu\n",sizeof(Message));
  printf("[info] Sizeof(Msg)=%zu\n",sizeof(Msg));
  printf("[info] Sizeof(ghs_config)=%zu\n",sizeof(ghs_config));

#define COMMS_Q_SZ 256

  //initialize the buffer used for input & output to get response messages from
  //those state machines. 
  //Use your own here ... 
  StaticQueue<Msg,COMMS_Q_SZ> buf;

  //initialize all the message-driven state machines that need msg callbacks.
  //In this case. Just GHS...
  GhsState<GHS_MAX_N,COMMS_Q_SZ> ghs(config.my_id);
  //here's the queue to/from ghs TODO: unify message types.
  StaticQueue<Msg,COMMS_Q_SZ> ghs_buf;
  //Now the stateful comms handler, to send/ receive those messages.
  //static allocated...
  auto& comms = DemoComms::inst().with_config(config);

  if (!comms.ok()){
    fprintf(stderr,"[error] Cannot create comms from config\n");
    return 1;
  }

  if (config.test==1){
    return do_test_and_die(comms,config);
  }

  if (config.command==ghs_config::START){
    initialize_ghs(ghs,config);
    size_t sent;
    auto ret = ghs.start_round(ghs_buf, sent);
    if (ret != GHS_OK){
      printf("[error] could not start ghs! (%d)\n", ret);
      return 1;
    }
  } 

  static bool wegood=true;

  //stop loop on sigint
  signal(SIGINT,[](int s){
      printf("...... Received shutdown, joining / killing all threads ..... \n");
      wegood=false;
      });

  if (config.wait_s>0){
    printf("[info] sleeping for %d seconds\n",config.wait_s);
    sleep(config.wait_s);
  }

  comms.start_receiver();
  while (wegood){

    Message in;
    //printf("[----] tick ... \n");

    bool ok = comms.get_next(in);

    if (ok){
      printf("[info] recv'd msg from %d to %d\n",in.agent_from, in.agent_to);

      //no shenanegans plz
      assert(in.agent_to==config.my_id);

      switch (in.payload.type){
        case PAYLOAD_TYPE_GHS:
          {
            //with static size checking:
            //Msg lm=from_bytes<GHS_MAX_MSG_SZ>(in.payload.bytes); 
            //or with compression
            Msg lm = from_bytes(in.payload.bytes, in.payload.size);

            //do recv work
            std::stringstream ss;
            ss<<lm;
            printf("[info] Received: %s\n",ss.str().c_str());
            size_t new_msg_ct;
            GhsError retval = ghs.process(lm,ghs_buf, new_msg_ct);
            if (retval != GHS_OK){
              printf("[error] could not call ghs.process()!");
              return 1;
            }
            printf("[info] # response msgs: %zu\n", new_msg_ct);
            printf("[info] GHS waiting: %zu\n", ghs.waiting_count());
            printf("[info] GHS delayed: %zu\n", ghs.delayed_count());
            printf("[info] GHS leader: %d parent: %d & level: %d\n", 
                ghs.get_leader_id(), 
                ghs.get_parent_id(),
                ghs.get_level());
            printf("[info] edges: %s\n", dump_edges(ghs).c_str());
            break;
          }
        default: { fprintf(stderr,"[error] unknown payload type: %d\n", in.payload.type); wegood=false; break;}
      }


    }

    while(ghs_buf.size()>0){
      printf("[info] Have %zu msgs to send\n", ghs_buf.size());
      Message out;
      Msg out_pld;

      if (OK!=ghs_buf.pop(out_pld)){
        wegood=false;
        break;
      }


      out.agent_to=out_pld.to;
      out.agent_from=out_pld.from;
      out.payload.type=PAYLOAD_TYPE_GHS;
      out.payload.size=sizeof(out_pld);

      //w/compression:
      size_t bsz = PAYLOAD_MAX_SZ;
      to_bytes(out_pld,out.payload.bytes,bsz);
      out.payload.size=bsz;
      //vs with size type checking
      //to_bytes<PAYLOAD_MAX_SZ>(out_pld, out.payload.bytes);

      int retval=comms.send(out);

      if (retval==0){
        std::stringstream ss;
        ss<<out_pld;
        printf("[info] Sent: %s\n",ss.str().c_str());
      } else if (retval>0){
        if (config.retry_connections){
          fprintf(stderr,"[error] Could not send, will retry: %d\n",retval);
          ghs_buf.push(out_pld);
          break;
        } else {
          fprintf(stderr,"[error] Could not send, assuming gone: %d\n",retval);
          kill_edge(ghs, config, out.agent_to);
        }
      } else {
        fprintf(stderr,"[error] system error. ghs-demo-comms.cpp should have printed an nng msg: %d\n",-retval);
      }
    }
    if (ghs.is_converged()){
      printf("Converged!\n");
      wegood=false;
    }

  }

  comms.stop_receiver();

  printf("[info] DemoComms stopped ... Exiting\n");

  //recv and reply OK
  //plug into ghs
  //for each resulting msg
  //send
  //reply

  return 0;
}
