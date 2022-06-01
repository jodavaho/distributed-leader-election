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
