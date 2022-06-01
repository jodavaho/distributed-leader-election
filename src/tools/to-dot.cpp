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
 * @file to-dot.cpp
 *
 */
#include <iostream>
#include <unordered_set>
#include <unordered_map>

int main(int argc, char** argv){
    int a,b,c;
    char d;

    std::unordered_set<int> nodes_seen;
    std::unordered_map<int,char> node_type;

    std::cout<<"graph result{"<<std::endl;
    std::cout<<"rankdir=BT;"<<std::endl;
    while(std::cin>>a>>b>>c>>d){
        if (a != b)
        {
            std::cout << a << " -- " << b;
            std::cout << " [label=\"" << c;
            std::cout << "\"];" << std::endl;
        }
        nodes_seen.insert(a);
        nodes_seen.insert(b);
        node_type[a]=d;
    }

    //print roots

    std::cout<<"{ rank=max"<<std::endl;
    for (auto node:nodes_seen){
        auto type = node_type[node];
        if (type == 'R'){
            std::cout<<node<< " [shape=box, rank=1];"<<std::endl;
        } 
    }
    std::cout<<"}"<<std::endl;


    //then the rest
    std::cout<<"{ rank=same"<<std::endl;
    for (auto node:nodes_seen){
        auto type = node_type[node];
        if (type != 'R' && type!='L'){
            std::cout<<node<< " [rank=2];"<<std::endl;
        }
    }
    std::cout<<"}"<<std::endl;

    //then leaves
    std::cout<<"{ rank=min"<<std::endl;
    for (auto node:nodes_seen){
        auto type = node_type[node];
        if (type == 'L'){
            std::cout<<node<< " [shape=triangle, rank=3];"<<std::endl;
        } 
    }
    std::cout<<"}"<<std::endl;

    std::cout<<"}"<<std::endl;


    std::cerr<<"to-dot done"<<std::endl;
    return 0;

}
