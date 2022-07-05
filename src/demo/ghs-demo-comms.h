/**
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
 * @file ghs-demo-comms.h
 *
 * @brief contains all the wire-level data structures and the Comms object to manage them.
 *
 */

#ifndef GHS_DEMO_COMMS
#define GHS_DEMO_COMMS 
#include "ghs-demo-config.h"
#include "ghs-demo-msgutils.h"
#include "ghs-demo-edgemetrics.h"
#include <dle/static_queue.h>
#include <nng/nng.h>//req_s, rep_s, msg
#include <cstring> //memcpy, memset
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>

#ifndef COMMS_DEMO_MAX_N
/// The max number of agents to demonstrate
#define COMMS_DEMO_MAX_N 8
#endif

#ifndef PAYLOAD_MAX_SZ
/// The max size of the payload to send over the wire
#define PAYLOAD_MAX_SZ 1024
#endif

/**
 * The demo:: namespace contains all the classes and structures for the ghs-demo executable
 */
namespace demo{

  /** 
   * @brief return codes for the Comms object
   */
  enum Errno{
    OK = 0,                 ///< No error
    ERR_BAD_PAYLOAD_SZ,     ///< Incorrect or no payload size
    ERR_NO_PAYLOAD_TYPE,    ///< Payload type not specified
    ERR_UNRECOGNIZED_PAYLOAD_TYPE, ///< Payload type unrecognized
    ERR_NULL_SRC,           ///< Bad from field or not set
    ERR_DEST_UNSET,         ///< Bad or unspecified Destination
    ERR_HANGUP,             ///< Connection was lost
    ERR_NNG,                ///< NNG returned an error code, please see logging output
  };

  /** 
   *
   * @brief the payload type
   */
  enum PayloadType
  {
    PAYLOAD_TYPE_NOT_SET =0,   ///< Not set (produces error)
    PAYLOAD_TYPE_CONTROL,      ///< Control message are for resetting internal state or commanding
    PAYLOAD_TYPE_METRICS,      ///< Metrics messages are for exchanging data about links
    PAYLOAD_TYPE_PING,         ///< Ping messages are used to benchmark links to gather metrics
    PAYLOAD_TYPE_GHS,          ///< message was intended for GHS, don't process, just send it
  };

//typedef uint8_t ControlCommand;
//#define CONTROL_TEST_MSG      1
//#define CONTROL_LATENCY       2
//#define CONTROL_START_GHS     12
//#define CONTROL_RESET_GHS     13

  /**
   * Reserved and not implemented yet. Any value is OK.
   */
  typedef uint64_t OptMask;

  /**
   * A two-byte header field that determines where the message is destined
   * This is different than agent_t (or maybe it is not -- your call!). 
   */
  typedef uint16_t Destination;
#define MESSAGE_DEST_UNSET -1


  /** 
   *
   * All outgoing messages have a sequence, since the network layer abstractions
   * guarantee eventual delivery, but not unique delivery, and GhsState is
   * sensitive to duplicated messages.
   *
   */
  typedef size_t SequenceCounter;

  /**
   * @brief a structure that defines source and destination for WireMessage objects
   *
   * Used by Comms objects to send to the appropriate endpoint
   *
   * @see Config
   * @see Comms
   */
  struct 
    //__attribute__((__packed__,aligned(1))) <-- messes with doxygen
    Header
    {
      /// Which agent id is this sent to?
      Destination agent_to=MESSAGE_DEST_UNSET;
      /// Which agent id is this from?
      uint16_t agent_from;
      /// What payload type do we carry?
      PayloadType type=PAYLOAD_TYPE_NOT_SET; 
      /// how big is the payload we carry?
      uint16_t payload_size=0;
    };

  /**
   * @brief a structure that contains control information for WireMessage objects
   *
   * For now only includes the sequence counter
   */
  struct 
    //__attribute__((__packed__,aligned(1))) <-- messes with doxygen
    Control
    {
      /// The sequence counter used to avoid duplicates or detect lost messagesa
      SequenceCounter sequence=0;
    };

  /**
   * @brief A wire-ready message structure that can encapsulate a variety of payloads for sending across the wire to an endpoint
   *
   *
   * Used to pass dle::GhsMsg payloads across the internet between agents, among other things
   *
   *
   * @see dle::GhsMsg
   */
  struct 
    //__attribute__((__packed__,aligned(1))) <-- messes with doxygen
    WireMessage
    {
      /// Msg control information
      Control control;
      /// Msg meta information
      Header header;
      /// The actual payload to send. Note only the first `N=demo::WireMessageHeader::payload_size` will be sent from the bytes.
      uint8_t bytes[PAYLOAD_MAX_SZ];

      /** 
       * The number of bytes to send over the wire. It is identical to sizeof(demo::Header) + sizeof(demo::Control) + header.payload_size
       */
      size_t size()  const
      { 
        return sizeof(demo::Header)
          +sizeof(demo::Control)
          +header.payload_size; 
      }
    };

  /**
   *
   * @brief a message passing class that uses nng, suitable for testing GhsState
   *
   * This class encapsulates the network layer for a demonstration of GHS over a real network.
   * It is initialized by populating a DemoConfig struct and calling with_config()
   *
   * After that, you can use send() and call get_next() at will to exchanges messages using nng_socket of type req-rep.
   */
  class Comms
  {
    public:
      Comms();
      ~Comms();
      bool ok();

      /**
       * Keep one static instance on hand. You're not required to use it, but we have it nonetheless
       * An usual way to initialize is to call Comms::inst().with_config()
       *
       */
      static Comms& inst();

      /**
       * Initialie all the internal data and communication structures with the given config information
       *
       * The intent is that you would extend this class with other `with_config()` options.
       *
       * @see ghs_read_cfg_file()
       * @see ghs_read_cfg_stdin()
       * @see ghs_read_cfg_cli()
       */
      Comms& with_config(Config &);

      /**
       * Sends a demo::WireMessage.
       * The demo::WireMessage contains all the destination and routing information in it, so this is a singular call that blocks then returns a status message.
       *
       * @return demo::WireMessageErrno denoting success or failure
       */
      Errno send(demo::WireMessage&, demo::OptMask=0);

      /**
       *
       * Starts a background process to copy incoming message into a buffer for later processing.
       *
       * This is implementation specific, I use plain old threads here.  Note, ZMQ does
       * this for you, but requires a lot of std:: and platform assumptions.
       *
       * So, we avoid *some of* that with nng, and one use of std::thread.
       */
      void start_receiver();
      /**
       *
       * Stops the background process by interrupting it after it's next timeout, then `join`ing the thread until it exits cleanly. This will block for a few seconds, but shouldn't fail. 
       *
       */
      void stop_receiver();

      /**
       * Returns true if there is a message waiting in the incoming buffer. 
       *
       * This will always return false until you call start_receiver()
       */
      bool has_msg();

      /**
       * If has_msg returns true, then this will copy the next message out of the buffer for you to process.
       */
      bool get_next(demo::WireMessage&);

      /**
       * This will block for a while, during which time it will repeatedly send and receive messages from all known endpoints to guage the throughput of the links. This information is used to populate the GhsState::mwoe() and Edge metric_t information.
       * However, the actual link metrics that are used are calculated by unique_link_metric_to()
       */
      void little_iperf();
      /**
       * This will block for a while, during which time it will synchronize with all known endpoints so that everyone uses the same link metric values. 
       *
       * However, the actual link metrics that are used are calculated by unique_link_metric_to()
       */
      void exchange_iperf();

      /**
       * Will dump link metric information to stdout
       */
      void print_iperf();

      /**
       * Returns the calculated and observed throughput to the given agent
       */
      Kbps kbps_to(const uint16_t agent_id) const;

      /**
       *
       * @brief **This function is really important for the working of your system**
       *
       * Calculates and returns a globally unique metric value that is suitable for use as a metric_t inside GhsState. 
       * The metric, once little_iperf() and exchange_iperf() complete and agent ids are globally unique, satisfies:
       *
       *   * symmetric: All pairs of agents use the same metric for the links between them
       *   * unique:    No edge metric is duplicated between any two edges.
       *
       * The implementation I used is sym_metric() but that's not required, compile in your own if you want. 
       *
       * @see sym_metric
       * @see dle::metric_t
       */
      CommsEdgeMetric unique_link_metric_to(const uint16_t agent_id) const;

    private:

      void validate_sockets();
      void read_loop();
      int recv(demo::WireMessage& buf);
      demo::Errno internal_send(demo::WireMessage &m, const char* endpoint, long &us_rt);

      dle::StaticQueue<demo::WireMessage,1024> in_q;
      bool read_continues;
      Config ghs_cfg;
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
}


#endif
