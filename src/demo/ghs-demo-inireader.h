#ifndef GHS_DEMO_CONFIG
#define GHS_DEMO_CONFIG

#define GHS_MAX_N 16
#define GHS_MAX_ENDPOINT_SZ 256
#define GHS_N_UNSET 0
#define GHS_ID_UNSET 255

#include <cstdint>
#include "ghs-demo-config.h"

//gcc: instead realize, there is no bool.
//g++: have a cookie, you'll feel right as rain
//assert ini format on everyone:
bool ghs_cfg_is_ok(ghs_config config);
void ghs_read_cfg_file(const char* fname, ghs_config* c);
void ghs_read_cfg_stdin(ghs_config*c);

//argp
void ghs_read_cfg_cli(int argc, char** argv, ghs_config* c);

#endif
