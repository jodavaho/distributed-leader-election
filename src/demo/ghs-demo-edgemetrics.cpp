/**
 *   @copyright 
 *   Copyright (c) 2021 California Institute of Technology (“Caltech”). 
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
 * @file ghs-demo-edgemetrics.cpp
 *
 */
#include "ghs-demo-edgemetrics.h"
#include <algorithm>

CommsEdgeMetric sym_metric(
    const uint16_t agent_to,
    const uint16_t agent_from,
    const Kbps kbps)
{
  uint16_t bigger = (uint16_t) std::max((int)agent_to, (int)agent_from);
  uint16_t smaller = (uint16_t) std::min((int)agent_to, (int)agent_from);
  CommsEdgeMetric ikbps=kbps;
  //how long would it take to send lots of data over this link?
  if (ikbps==0){ //or, if ikbps == 1, really
    ikbps=std::numeric_limits<CommsEdgeMetric>::max();//too long
  } else {
    ikbps =(CommsEdgeMetric)  ((double) std::numeric_limits<CommsEdgeMetric>::max() / (double) kbps);//not so long (min=1 "unit")
  }
  return 
     ((uint64_t)bigger<<16)
    +((uint64_t)smaller<<0)
    +((uint64_t)ikbps <<32) 
    ;
}
