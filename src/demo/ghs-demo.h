/**
 * @file ghs-demo.h
 * @brief provides an "executable class" for running leader election on multiple machines
 *
 */

#include <signal.h>
#include <unistd.h> //sleep
#include <sstream> //better than iostream
#include <cassert>

#include "ghs-demo-config.h"
#include "ghs-demo-msgutils.h"
#include "ghs-demo-comms.h"

#include "ghs/ghs.h"
#include "ghs/ghs_printer.h" //dump_edges
#include "ghs/msg_printer.h" //for printing GHS msgs.
#include "ghs/agent.h"
#include "ghs/edge.h"
#include "seque/static_queue.h"

using le::ghs::GhsState;
using le::ghs::metric_t;
using le::ghs::agent_t;
using le::ghs::Edge;
using le::ghs::Msg;

///
namespace demo{

  /**
   * @brief **The main demo logic** for executing le::ghs::GhsState across a network
   *
   * A configurable multi-agent/multi-process executable built on nng that uses le::ghs::GhsState
   *
   */
  class GhsDemoExec{
    public:
      /**
       * @brief A single entry point function that should be called from main()
       *
       * The main loop consists of:
       *
       * 1. Reading demo::Config information about the peers and their tcp endpoints from an ini file and from stdin
       * 2. Initializing all the nng sockets inside a demo::Comms object from that config
       * 3. Using Comms::little_iperf() and Comms::exchange_iperf() to create link metrics
       * 4. Populating a le::ghs::GhsState object from the Config object and link information gathered by demo::Comms
       * 5. Calling le::ghs::GhsState::start_round() to get the first set of messages, and feeding those into Comms
       * 6. Spinning on Comms::has_msg() and calling Comms::get_next() to retrieve a message, then pushing that message payload into le::ghs::GhsState::process() to get the next set of message to send
       * 7. Continuing that process until le::ghs::GhsState::is_converged() returns true
       * 8. Printing stuff
       *
       * If you want to replicate this, study this loop, and pay close attention to how little_iperf() does its work **and especially sym_metric()**.
       *
       * @see demo::Config
       * @see le::ghs::GhsState
       * @see le::ghs::Msg
       * @see demo::WireMessage
       * @see sym_metric()
       *
       */
      int do_main(int argc,char**argv);
  };

  /// Helper function to build edges from configuration information
  template<size_t AN, size_t QN>
    void initialize_ghs(GhsState<AN,QN>& ghs, Config& cfg, Comms& c)
    {
      ghs.reset();
      for (int i=0;i<cfg.n_agents;i++){
        if (i!=cfg.my_id){
          metric_t link_wt = (metric_t) c.unique_link_metric_to(i);
          ghs.set_edge( { (agent_t)i, (agent_t)cfg.my_id, UNKNOWN, link_wt, } ); 
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

  /// Helper function to remove an edge from the GhsState, but trying to do sanely and without interrupting the execution flow
  template<size_t AN, size_t QN>
    void kill_edge(GhsState<AN,QN>& ghs, Config & cfg, uint8_t agent_to)
    {
      bool resp_req=false, waiting_for=false;
      ghs.set_edge( {agent_to, (agent_t) cfg.my_id, DELETED, 0} );

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


  /// Run a quick little_iperf() round to check connectivity
  int do_test_and_die(Comms& comms, Config &config){
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

  int GhsDemoExec::do_main(int argc, char** argv)
  {

    demo::Config config;
    static const size_t COMMS_Q_SZ=256;

    demo::read_cfg_stdin(&config);
    demo::read_cfg_cli(argc,argv,&config);

    printf("[info] Done configuring for id=%d... \n",config.my_id);

    if (!demo::cfg_is_ok(config)){
      return 1;
    }


    //initialize the buffer used for input & output to get response messages from
    //those state machines. 
    //Use your own here ... 
    seque::StaticQueue<Msg,COMMS_Q_SZ> buf;

    //initialize all the message-driven state machines that need msg callbacks.
    //In this case. Just GHS...
    le::ghs::GhsState<MAX_N,COMMS_Q_SZ> ghs(config.my_id);

    //here's the queue to/from ghs TODO: unify message types.
    seque::StaticQueue<Msg,COMMS_Q_SZ> ghs_buf;

    //Now the stateful comms handler, to send/ receive those messages.
    //static allocated...
    auto& comms = demo::Comms::inst().with_config(config);

    if (!comms.ok()){
      printf("[error] Cannot create comms from config\n");
      return 1;
    }

    if (config.test==1){
      return demo::do_test_and_die(comms,config);
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

    if (config.command==demo::Config::START){
      demo::initialize_ghs(ghs,config,comms);
      size_t sent;
      auto ret = ghs.start_round(ghs_buf, sent);
      if (ret != le::ghs::OK){
        printf("[error] could not start ghs! (%d)\n", ret);
        return 1;
      }
    } 


    while (wegood){

      demo::WireMessage in;
      //retrieve next message from background reader.
      bool ok = comms.get_next(in);

      if (ok){
        printf("[info] recv'd msg from %d to %d\n",in.header.agent_from, in.header.agent_to);

        //no shenanegans plz
        assert(in.header.agent_to==config.my_id);

        //there might be other types, like link metrics, control messages, or
        //other subsystems to prod. etc
        switch (in.header.type){
          case demo::PAYLOAD_TYPE_CONTROL:
            { 
              break;
            }
          case demo::PAYLOAD_TYPE_GHS:
            {
              //with static size checking:
              //Msg payload_msg=from_bytes<MAX_MSG_SZ>(in.bytes); 
              //or with compression / variable sizes
              Msg payload_msg = from_bytes(in.bytes, in.header.payload_size);
              //push msg to the subsystem
              std::stringstream ss;
              ss<<payload_msg;
              printf("[info] received GHS msg: %s\n",ss.str().c_str());
              size_t new_msg_ct=0;
              le::ghs::Retcode retval = ghs.process(payload_msg,ghs_buf, new_msg_ct);
              if (retval != le::ghs::OK){
                printf("[error] could not call ghs.process():%s",le::ghs::strerror(retval));
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
        demo::WireMessage out;
        le::ghs::Msg out_pld;

        printf("[info] Have %zu msgs to send\n", ghs_buf.size());
        if (seque::OK!=ghs_buf.pop(out_pld)){
          wegood=false;
          break;
        }
        out.header.agent_to=out_pld.to;
        out.header.agent_from=out_pld.from;
        out.header.type=demo::PAYLOAD_TYPE_GHS;
        out.header.payload_size=sizeof(out_pld);
        size_t bsz = sizeof(out_pld);
        to_bytes(out_pld,out.bytes,bsz);
        assert(bsz==sizeof(out_pld));
        demo::Errno retval=comms.send(out); 

        if (retval==demo::OK){
          std::stringstream ss;
          ss<<out_pld;
          printf("[info] Sent: %s\n",ss.str().c_str());
        } else if (retval == demo::ERR_NNG || retval == demo::ERR_HANGUP){
          if (config.retry_connections){
            printf("[error] Could not send, will retry: %d\n",retval);
            ghs_buf.push(out_pld);
            break;
          } else {
            printf("[error] Could not send, assuming gone: %d\n",retval);
            kill_edge(ghs, config, out.header.agent_to);
          }
        } else {
          printf("[error] demo error. We may have populated a message incorreclty %d\n",-retval);
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
    printf("[info] Comms stopped ... Exiting\n");

    return 0;
  }
}
