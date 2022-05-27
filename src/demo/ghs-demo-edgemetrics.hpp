#ifndef GHS_DEMO_EDGEMETRICS
#define GHS_DEMO_EDGEMETRICS

#include <cstdint>

typedef uint64_t CommsEdgeMetric;
typedef uint32_t Kbps;

CommsEdgeMetric sym_metric(
    const uint16_t agent_id,
    const uint16_t agent_from,
    const Kbps kbps);

#endif
