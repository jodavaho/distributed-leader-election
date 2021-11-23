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