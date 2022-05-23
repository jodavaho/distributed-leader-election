#ifndef GHS_DEMO_INIREADER
#define GHS_DEMO_INIREADER

#define GHS_MAX_N 16
#define GHS_MAX_ENDPOINT_SZ 256
#define GHS_N_UNSET 0
#define GHS_ID_UNSET 255

#include <cstdint>

struct ghs_config
{
  int wait_s= 0 ;
  uint8_t n_agents=GHS_N_UNSET;
  char endpoints[GHS_MAX_N][GHS_MAX_ENDPOINT_SZ]={0};
  uint8_t my_id=GHS_ID_UNSET;
  enum {
    LOAD=0,
    TEST,
    START,
  }command=LOAD;
  bool retry_connections=false;
  bool test = false;
};

//gcc: instead realize, there is no bool.
//g++: have a cookie, you'll feel right as rain
bool ghs_cfg_is_ok(ghs_config config);
void ghs_read_cfg_file(const char* fname, ghs_config* c);
void ghs_read_cfg_stdin(ghs_config*c);

void ghs_read_cfg_cli(int argc, char** argv, ghs_config* c);

#endif
