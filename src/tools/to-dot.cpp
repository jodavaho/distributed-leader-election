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