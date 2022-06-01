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
 * @file ghs-demo-config.h
 *
 * @brief contains all the Config objects and methods for configuring demo::Comms
 *
 */

#ifndef DEMO_CONFIG
#define DEMO_CONFIG

/// The maximum number of agents the demo can accomodate
#define MAX_N 8

/// The max strlen() of the endpoint
#define MAX_ENDPOINT_SZ 256

/// A special value for N that means "not set yet"
#define N_UNSET 0

/// A special value for an ID that means "not set yet"
#define ID_UNSET -1

#include <cstdint>

///
namespace demo{
  /** 
   *
   * @brief A struct that holds the union of all configuration variables. 
   *
   */
  struct Config
  {
    /// The time to wait before starting the main algorithm
    int wait_s= 0 ;

    /// The number of agents currently loaded
    int n_agents=N_UNSET;

    /// The endpoint for each agent, as read from the config
    char endpoints[MAX_N][MAX_ENDPOINT_SZ]={0};

    /// A bool to determine if we should do a test-and-die routine or not
    int test=0;

    /// This agent's id
    int my_id=ID_UNSET;

    /// A special command enumeration
    enum {
      LOAD=0,      ///< Load config and do nothing else
      TEST,        ///< run test-and-die routine 
      START,       ///< Start MST construction after specified delay
    }command=LOAD;
    ///< The start mode

    ///< If we fail to dial up an agent, should we retry later (true) or drop the message (false)
    bool retry_connections=false;


  };

  /**
   * Checks to make sure the values populated in the config are sane
   *
   * @return true if it is ok
   * @return false if not
   */
  bool cfg_is_ok(Config config);

  /**
   * Reads the config from an ini-formatted file (if you compiled in
   * ghs-demo-inireader.cpp, but you are free to implement this function however
   * you see fit if not
   */
  void read_cfg_file(const char* fname, Config* c);

  /**
   * Reads the config from an ini-formatted file that is piped over stdin 
   * (if you compiled in ghs-demo-inireader.cpp, but you are free to implement
   * this function however you see fit if not)
   *
   * Like this:
   *
   * `< some_cfg.ini ghs-demo [other switches / args]`
   */
  void read_cfg_stdin(Config*c);

  /** 
   * Populates config variables based on command line switches.
   * Uses arpg.h.
   * See ghs-demo-clireader.cpp
   */
  void read_cfg_cli(int argc, char** argv, Config* c);

}

#endif
