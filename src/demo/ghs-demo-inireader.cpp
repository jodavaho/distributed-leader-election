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
 * @file ghs-demo-inireader.cpp
 *
 */

// linux:
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <unistd.h>
#include <poll.h>
#include <cerrno>
// apt:
#include <ini.h> // use get_deps.sh
// ghs:
#include "ghs-demo-config.h"

namespace demo{

  //there's a perfectly good C++ api too
  //https://github.com/benhoyt/inih/blob/master/examples/INIReaderExample.cpp
  static int read_cfg_item(void* user, const char* section, const char* name, const char * value);

  void read_cfg_file(const char* fname, Config*config){
    int error = ini_parse(fname,read_cfg_item,(void*)config);
    printf("return: %d\n",error);
  }


  /* 
   * From INI.h:
   * Parse given INI-style file. May have [section]s, name=value pairs
   * (whitespace stripped), and comments starting with ';' (semicolon). Section
   * is "" if name=value pair parsed before any section heading. name:value
   * pairs are also supported as a concession to Python's configparser.
   * For each name=value pair parsed, call handler function with given user
   * pointer as well as section, name, and value (data only valid for duration
   * of handler call). Handler should return nonzero on success, zero on error.
   * Returns 0 on success, line number of first error on parse error (doesn't
   * stop on first error), -1 on file open error, or -2 on memory allocation
   * error (only when INI_USE_STACK is zero).
   */
  int read_cfg_item(void* user, const char* section, const char* name, const char * value)
  {

    //this is more than enough, but note this is coupled to MAX_N
    static char agent_str[32]={0};

    Config * config = (Config*) user;

    if (!user){
      printf("[warn] ignored %s.%s=%s\n",section,name,value);
      return 0;
    }

    //process "ghs" section
    if (strcmp(section,"ghs")==0){
      //process agent.
      assert(config->n_agents < MAX_N);
      //create a string for comparison, from the next agent id,
      //which, becasue they are zero-indexed, is simply 
      //the size of the set of current agents.
      snprintf(agent_str,32,"%d",config->n_agents);

      //we currently only accept <id>=<hostname> pairs
      //of the form [agent], 0=<hostname>, so "0" is the name string
      if (strcmp(name,agent_str)==0){
        //copy over value
        snprintf(config->endpoints[config->n_agents],MAX_ENDPOINT_SZ,"%s",value);

        //print for funsies
        printf("[info] set '%s.%s'=%s (%d)\n",section,name,config->endpoints[config->n_agents], config->n_agents);

        //note the new agent
        config->n_agents++;
        //OK, it worked
        return 1;

        //}else if (...) {
        //If you had other things to parse, put them here...
    } else if (strcmp(name,"retry_connections")==0)
      {
        if (strcmp(value,"true")==0){
          config->retry_connections=true;
          return 1;
        }
        if (strcmp(value,"1")==0){
          config->retry_connections=true;
          return 1;
        }
        if (strcmp(value,"0")==0){
          config->retry_connections=false;
          return 1;
        }
        if (strcmp(value,"false")==0){
          config->retry_connections=false;
          return 1;
        }
        return 0;
      }

    } else if (strcmp(section,"runtime")==0)
    {
      if (strcmp(name,"debug")==0){
        printf("[warn] ok, but unimplemented: %s.%s=%s\n",section,name,value);
        return 1;
      } 

      if (strcmp(name,"auto_start")==0){
        if (strcmp(value,"true")==0){
          config->command=Config::START;
          printf("[info] auto-start = true\n");
          return 1;
        } else if (strcmp(value,"false")==0){
          config->command=Config::START;
          printf("[info] auto-start = false\n");
          return 1;
        } else {
          printf("[warn] unrecognized value: %s.%s=%s\n",section,name,value);
          return 0;
        }
      } 

      if(strcmp(name,"wait_time_seconds")==0){
        errno=0;
        double val = strtof(value,0);
        int err = errno;
        if (val==0.0 && err!=0){
          printf("[warn] Error converting %s to a float: %s\n",value,strerror(err));
          return 0;
        }
        printf("[info] config says wait %f seconds\n",val);
        config->wait_s=val;
        return 1;
      }


      return 0;
    } 
    printf("[warn] unrecognized: %s.%s=%s\n",section,name,value);
    return 0;
  }

  /**
   * Validate the config file, and return 'false' if the config is misread (some
   * variable unset) or has semantic errors. For example if my_id is >=
   * num_agents, or is <0. 
   */
  bool cfg_is_ok(Config config){

    if (config.my_id==ID_UNSET){
      fprintf(stderr,"[error] Invalid my_id!\n");
      return false;
    }

    if (config.n_agents==N_UNSET){
      fprintf(stderr,"[error] Error determining # agents or their endpoints in config\n");
      return false;
    }

    for (int i=0;i<config.n_agents;i++){
      if (strlen(config.endpoints[i])==0){
        fprintf(stderr,"[error] Error parsing endpoint for agent %d",i);
        return false;
      }
    }

    if (config.my_id>=config.n_agents || config.my_id<0){
      fprintf(stderr,"[error] Invalid my_id =%d for n_agents =%d\n",config.my_id, config.n_agents);
      return false;
    }
    return true;

  }

  void read_cfg_stdin(Config*config)
  {
    //poll(2): int poll(struct pollfd *fds, nfds_t nfds, int timeout);
    struct pollfd fd;
    int ret;
    fd.fd=0;
    fd.events=POLLIN;
    ret = poll(&fd,1,0);
    if (ret==1){
      printf("[info] reading from stdin\n");
      int error = ini_parse_file(stdin, read_cfg_item,(void*)config);
      printf("[warn] suspicous line:%d\n",error);
    } else {
      printf("[warn] error reading stdin or no stdin\n");
    }
  }
}
