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
 * @file middle-leader.cpp
 *
 */
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <assert.h>

#include "comms.hpp"

int main(int argc, char** argv){

    Comms comms;
    comms.set_logfile("STLeader.msgs");

    int a,b,c;
    char d;
    int count=0;
    std::unordered_map<int, std::vector<int> > edges;
    std::unordered_map<int,int> next;
    std::unordered_map<int, std::unordered_map<int,int>> wts;

    while(std::cin>>a>>b>>c>>d){
        next[a]=b; // for printing like they gave it to us, not for processing
        if (a!=b){
            edges[b].push_back(a);
            edges[a].push_back(b);
        }
        wts[a][b]=c;
        wts[b][a]=c;
        std::cerr<<a<<"->"<<b<<std::endl;
        count++;
    }
    if (count==0){
        std::cerr<<argv[0]<<"Read no tree!"<<std::endl;
    }

    std::vector<std::unordered_set<int>> not_received(count,std::unordered_set<int>());

    for (int i=0;i<count;i++){
        //make sure we keep track of who we've heard from:
        for (const auto child: edges[i]){
            assert (child!=i);
            not_received[i].insert(child);
        }

        //and if we're a leaf node, start the opt-out procedure for leadership
        if (edges[i].size()==1){
            //I AM LEAF
            int parent_id = edges[i][0];
            assert (parent_id!=i);
            comms.send(Msg::Type::NOT_IT,parent_id,i);
            not_received[i].erase(parent_id);
        }
    }

    int root = -1;
    while (comms.waiting()>0 && root == -1){
        //unpack msg
        auto m = comms.get();
        int nid = m.to;
        int sender = m.from;
        auto type = m.type;
        auto data = m.data;
        
        if (type != Msg::Type::NOT_IT){
            //oh what the hell happened
            std::cerr<<"How did a non-not_it message get on here?";
            return -1;
        }

        if (not_received[nid].size() == 0){
            //well hell, sender stuck us. 
            root = std::max(nid,sender);
            break;
        } 

        not_received[nid].erase(sender);
        std::cerr<<"node "<<nid<<" has now not heard from "<<not_received[nid].size()<<" others"<<std::endl;
        if (not_received[nid].size()==0){
            //we're leader, nobody to nominate
            root = nid; 
            break;
        } 
        if (not_received[nid].size()==1){
            //we can safely say not-it
            int stuckee = *not_received[nid].begin();
            comms.send(Msg::Type::NOT_IT, stuckee, nid, {});
            not_received[nid].erase(stuckee);
        }
    }

    std::cerr << "Root: " << root << std::endl;
    std::cerr << "Dumping original graph with leaf/trunk/root designations" << std::endl;
    for (int i = 0; i < count; i++)
    {
        std::cout << i << " " << next[i] << " " << wts[i][next[i]] << " ";
        if (edges[i].size() == 1)
        {
            std::cout << "L" << std::endl;
        }
        else if (root == i)
        {
            //did you know root has one child, even if it is alone?
            std::cout << "R" << std::endl;
        }
        else
        {
            std::cout << "T" << std::endl;
        }
    }
    std::cerr<<"middle-leader done"<<std::endl;
    return 0;
}
