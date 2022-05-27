#ifndef GHS_DEMO_CONFIG
#define GHS_DEMO_CONFIG

#define GHS_MAX_N 8
#define GHS_MAX_ENDPOINT_SZ 256
#define GHS_N_UNSET 0
#define GHS_ID_UNSET -1

#include <cstdint>

struct ghs_config
{
  int wait_s= 0 ;
  int n_agents=GHS_N_UNSET;
  char endpoints[GHS_MAX_N][GHS_MAX_ENDPOINT_SZ]={0};
  int test=0;
  int my_id=GHS_ID_UNSET;
  enum {
    LOAD=0,
    TEST,
    START,
  }command=LOAD;
  bool retry_connections=false;
  int latency_period_ms=0;

};

//gcc: instead realize, there is no bool.
//g++: have a cookie, you'll feel right as rain
bool ghs_cfg_is_ok(ghs_config config);
void ghs_read_cfg_file(const char* fname, ghs_config* c);
void ghs_read_cfg_stdin(ghs_config*c);

void ghs_read_cfg_cli(int argc, char** argv, ghs_config* c);

#endif
