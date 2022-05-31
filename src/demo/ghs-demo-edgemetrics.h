/**
 * @file ghs-demo-edgemetrics.h
 * @brief provides the main comms metric calculation.
 */
#ifndef GHS_DEMO_EDGEMETRICS
#define GHS_DEMO_EDGEMETRICS

#include <cstdint>

/**
 * @brief a 64-bit edge metric. 64 bits is important!
 */
typedef uint64_t CommsEdgeMetric;

/**
 * @brief a 32 bit throughput measuremeng. 32 bits is important!
 */
typedef uint32_t Kbps;

/**
 * @brief **Dark magic** to provide a symmetric, unique link-metric based on throughput and the two agent ids
 *
 * Required for proper execution of le::ghs::GhsState.
 * The 64bit return value is composed of 32 bits of throguhput left-shifted by 32 bits, OR-masked with the larger agent id left shifted 16 bits, finally OR-masked with the lower agent id.
 *
 * The resulting "hashy-metric" is:
 *
 *    * Unique, in that even with identical throughputs, no two pairs of agents will have identical comms metrics
 *    * Symmetrical, in that both agents, if they agree on throughput can agree on the metric to use
 *    * Dominated by throughput, so no two agent ids are somehow more important than raw transmission power
 *
 *  @see le::ghs::metric_t
 *  @see le::ghs::GhsState
 *  @see demo::Comms::little_iperf()
 */
CommsEdgeMetric sym_metric(
    const uint16_t agent_id,
    const uint16_t agent_from,
    const Kbps kbps);

#endif
