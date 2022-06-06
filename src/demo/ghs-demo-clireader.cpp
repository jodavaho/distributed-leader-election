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
 * @file ghs-demo-clireader.cpp
 *
 */
#include "ghs-demo-config.h"
#include <argp.h>
#include <cstdlib>

/// 
namespace demo{

  /// for argp
  static const char KEY_MY_ID='i';
  /// for argp
  static const char KEY_START='s';
  /// for argp
  static const char KEY_WAIT='w';
  /// for argp
  static const char KEY_TEST='t';
  /// for argp
  const char *argp_program_bug_address = "Joshua Vander Hook <hook@jpl.nasa.gov>";

  static error_t parse_it(int key, char *arg, struct argp_state *state) 
  {
    //argp  https://www.gnu.org/software/libc/manual/html_node/Argp.html
    Config* p = (Config*) state->input;
    switch (key)
    {
      case KEY_MY_ID:{
                       p->my_id = (uint8_t) atoi(arg);
                       printf("[debug] set 'id'='%s' (%d)\n",arg,p->my_id);
                       break;
                     }
      case KEY_START:{
                       p->command= Config::START;
                       break;
                     }
      case KEY_WAIT:{
                      p->wait_s = atof(arg);
                      if (p->wait_s != 0.0 || errno==0){
                        printf("[debug] set 'wait'='%s' (%f)\n",arg,p->wait_s);
                      } else {
                        printf("[debug] unable to set 'wait' to '%s'\n",arg);
                        perror(0);
                      }
                      break;
                    }
      case KEY_TEST:{
                      p->test=1;
                      printf("[debug] set 'test'='1' \n");
                      break;
                    }
      case ARGP_KEY_NO_ARGS: {break;}
      default:{return ARGP_ERR_UNKNOWN;}
    };

    return 0;
  }

  void read_cfg_cli(int argc, char** argv,Config* config)
  {
    //argp  https://www.gnu.org/software/libc/manual/html_node/Argp.html
    static struct argp_option options[]=
    {
      {"id", KEY_MY_ID, "MY_ID", 0, "The ID to assume, which must be included in the config"},
      {"start", KEY_START, 0, OPTION_ARG_OPTIONAL, "Start the algorithm."},
      {0, KEY_WAIT, "X", 0, "wait X seconds before sending first round of messages (good to help with initializing connections)"},
      {"test", KEY_TEST, 0, 0, "for 10 seconds, blast msgs (if agent 0), or wait for msgs (otherwise)"},
      {0}//null terminated, of course
    };

    struct argp argp = {
      options,
      parse_it,
      "",
      "A GHS demo wrapper. It might even work!"
    };

    argp_parse(&argp,argc,argv,ARGP_IN_ORDER,0,(argp_state*)config);
    printf("[debug] id=%d\n",config->my_id);
  }
}
