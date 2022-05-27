
#include <signal.h>
#include <unistd.h> //sleep
#include <sstream> //better than iostream
#include <cassert>

#include "ghs-demo-config.h"
#include "ghs-demo-msgutils.hpp"
#include "ghs-demo-comms.hpp"
#include "ghs/ghs_printer.hpp" //dump_edges

#include "ghs/ghs.hpp"
#include "ghs/msg_printer.hpp" //for printing GHS msgs.
#include "ghs/graph.hpp"
#include "seque/seque.hpp"

template<size_t AN, size_t QN>
void initialize_ghs(GhsState<AN,QN>& ghs, ghs_config& cfg, DemoComms& c)
{
  ghs.reset();
  for (int i=0;i<cfg.n_agents;i++){
    if (i!=cfg.my_id){
      EdgeMetric link_wt = (EdgeMetric) c.unique_link_metric_to(i);
      ghs.set_edge( { (AgentID)i, (AgentID)cfg.my_id, UNKNOWN, link_wt, } ); 
      printf("[info] Set edge: %d from %d (check=%d)\n",
          i,
          cfg.my_id,
          ghs.has_edge(i));
      Edge e;
      ghs.get_edge(i,e);
      printf("[info] (%d<--%d, %d %lu)\n",
          e.peer,e.root,e.status,e.metric_val);
    }else{
      printf("[info] Ignoring: %d (it's me)\n", i);
    }
  }
}

template<size_t AN, size_t QN>
void kill_edge(GhsState<AN,QN>& ghs, ghs_config & cfg, uint8_t agent_to)
{
  bool resp_req=false, waiting_for=false;
  ghs.set_edge( {agent_to, (AgentID) cfg.my_id, DELETED, 0} );

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
  printf("[info] running connectivity test after %ds!\n",config.wait_s);
  sleep(config.wait_s);
  comms.start_receiver();
  sleep(1);
  comms.little_iperf();
  sleep(1);
  printf("[info]================= measured\n");
  comms.print_iperf();
  sleep(1);
  printf("[info]================= exchanging \n");
  comms.exchange_iperf();
  sleep(1);
  printf("[info]================= post-exchange\n");
  comms.print_iperf();
  sleep(1);
  printf("[info]================= again\n");
  comms.print_iperf();
  comms.stop_receiver();
  return 0;
}

//each node will have a listener / rep, where it can receive msgs. 
//each node will also set up a req/dialer to send msgs to other nodes. 
int main(int argc, char** argv){

  ghs_config config;
  static const size_t COMMS_Q_SZ=256;

  //can we read configs without allocating memory!?
  //Yes we can!
  ghs_read_cfg_stdin(&config);
  ghs_read_cfg_cli(argc,argv,&config);

  printf("[info] Done configuring for id=%d... \n",config.my_id);

  if (!ghs_cfg_is_ok(config)){
    return 1;
  }


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
    printf("[error] Cannot create comms from config\n");
    return 1;
  }

  if (config.test==1){
    return do_test_and_die(comms,config);
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
  comms.little_iperf();
  comms.print_iperf();
  sleep(1);
  comms.exchange_iperf();
  comms.print_iperf();
  sleep(1);
  comms.print_iperf();

  if (config.command==ghs_config::START){
    initialize_ghs(ghs,config,comms);
    size_t sent;
    auto ret = ghs.start_round(ghs_buf, sent);
    if (ret != GHS_OK){
      printf("[error] could not start ghs! (%d)\n", ret);
      return 1;
    }
  } 


  while (wegood){

    Message in;
    //retrieve next message from background reader.
    bool ok = comms.get_next(in);

    if (ok){
      printf("[info] recv'd msg from %d to %d\n",in.header.agent_from, in.header.agent_to);

      //no shenanegans plz
      assert(in.header.agent_to==config.my_id);

      //there might be other types, like link metrics, control messages, or
      //other subsystems to prod. etc
      switch (in.header.type){
        case PAYLOAD_TYPE_CONTROL:
          { 
            break;
          }
        case PAYLOAD_TYPE_GHS:
          {
            //with static size checking:
            //Msg payload_msg=from_bytes<GHS_MAX_MSG_SZ>(in.bytes); 
            //or with compression / variable sizes
            Msg payload_msg = from_bytes(in.bytes, in.header.payload_size);
            //push msg to the subsystem
            std::stringstream ss;
            ss<<payload_msg;
            printf("[info] received GHS msg: %s\n",ss.str().c_str());
            size_t new_msg_ct=0;
            GhsError retval = ghs.process(payload_msg,ghs_buf, new_msg_ct);
            if (retval != GHS_OK){
              printf("[error] could not call ghs.process():%s",ghs_strerror(retval));
              return 1;
            }
            printf("[info] # response msgs: %zu\n", new_msg_ct);
            printf("[info] GHS waiting: %zu, delayed: %zu, leader: %d, parent: %d, level: %d\n", 
                ghs.waiting_count(),
                ghs.delayed_count(),
                ghs.get_leader_id(), 
                ghs.get_parent_id(),
                ghs.get_level());
            printf("[info] Edges: %s\n",
                dump_edges(ghs).c_str());
            break;
          }
        default: { printf("[error] unknown payload type: %d\n", in.header.type); wegood=false; break;}
      }


    }


    //now process the outgoing msgs 
    //ghs example, others are similar:
    //You don't really *need* to wrap messages like this ... 
    while(ghs_buf.size()>0){
      Message out;
      //ghs msgs:
      Msg out_pld;

      printf("[info] Have %zu msgs to send\n", ghs_buf.size());
      if (OK!=ghs_buf.pop(out_pld)){
        wegood=false;
        break;
      }
      out.header.agent_to=out_pld.to;
      out.header.agent_from=out_pld.from;
      out.header.type=PAYLOAD_TYPE_GHS;
      out.header.payload_size=sizeof(out_pld);
      size_t bsz = sizeof(out_pld);
      to_bytes(out_pld,out.bytes,bsz);
      assert(bsz==sizeof(out_pld));
      int retval=comms.send(out);

      if (retval==0){
        std::stringstream ss;
        ss<<out_pld;
        printf("[info] Sent: %s\n",ss.str().c_str());
      } else if (retval>0){
        if (config.retry_connections){
          printf("[error] Could not send, will retry: %d\n",retval);
          ghs_buf.push(out_pld);
          break;
        } else {
          printf("[error] Could not send, assuming gone: %d\n",retval);
          kill_edge(ghs, config, out.header.agent_to);
        }
      } else {
        printf("[error] system error. ghs-demo-comms.cpp should have printed an nng msg: %d\n",-retval);
      }
    }

    if (ghs.is_converged()){
      printf("Converged!\n");
      wegood=false;
    }

  }

  printf("[info] waiting a bit for cleanup ... \n");
  sleep(3);
  comms.stop_receiver();
  printf("[info] DemoComms stopped ... Exiting\n");

  return 0;
}
