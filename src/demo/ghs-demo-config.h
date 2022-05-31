/**
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
