#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <vector>
#include <string>
#include <assert.h>
#include <math.h>
#include <unordered_map>

#include "comms.hpp"

#define MAX_AGENTS 1024
using std::cerr;
using std::cerr;
using std::endl;
using std::cin;


struct Edge
{
	size_t to, from;
	size_t weight;
};

enum EdgeClass
{
	UNKNOWN_EDGE = 0,
	DISCARDED = -1,
	MST = 1
};

std::ostream& operator<<(std::ostream& s, const Edge& e){
	return s<<e.from<<" "<<e.to<<" "<<e.weight;
}


/**
 * Keeping the i/o for now, but need to replace guts with ghs.cpp
 */
int main(int argc, char *argv[]) 
{
	Comms comms;
	comms.set_logfile("ghs.msgs");

	srand(99);
	size_t num_agents = 0;
	std::unordered_map<size_t, std::unordered_map<size_t, size_t>> connectivity_scratch_pad;
	{
		std::unordered_set<size_t> agents;
		size_t a, b, c;
		char d;
		while (std::cin >> a >> b >> c >> d)
		{
			std::cerr<<"Found: "<< a << " "<< b << " " << c<<std::endl;
			connectivity_scratch_pad[a][b] = c;
			agents.insert(a);
			agents.insert(b);
		}
		num_agents = agents.size();
		for (size_t i=0;i<num_agents;i++){
			//make sure they were 0 to N-1
			assert(agents.find(i)!=agents.end());
			agents.erase(i);
		}
		//and none others
		assert(agents.size()==0);
	}

	//initialize the memory for the nodes. For each of these, variables X, X[i]
	//is the memory for that node. So a NxN matrix just means the node needs N
	//things to rememver. 
	//Yes, I could have had a list of N structs containing that memory. I don't
	//know why I didn't. 
	auto edge_list = std::vector<std::vector<Edge>>(num_agents, std::vector<Edge>(num_agents));
	auto best_edge = std::vector<Edge>(num_agents);
	auto waiting_for = std::vector<size_t> (num_agents,0);
	auto part_leader= std::vector<size_t> (num_agents,0);
	auto parent = std::vector<size_t> (num_agents,0);
	auto edge_class = std::vector<std::vector<EdgeClass>>(num_agents,std::vector<EdgeClass>(num_agents,EdgeClass::UNKNOWN_EDGE));
	auto connectivity_matrix = std::vector<std::vector<double>>(num_agents,std::vector<double>(num_agents,0.0));

	for (size_t i = 0; i < num_agents; i++)
	{
		//which partition is each node part of?
		part_leader[i] = i; //own partition so far
		//who is parent in the MST?
		parent[i] = i; //i_am_root!
		edge_list[i].reserve(num_agents);
		for (size_t j=0;j<num_agents;j++){
			edge_list[i][j]=Edge{i,j,connectivity_scratch_pad[i][j]};
			connectivity_matrix[i][j] = connectivity_scratch_pad[i][j];
		}
 
		std::sort(std::begin(edge_list[i]), std::end(edge_list[i]),
				[] (const Edge& lhs, const Edge& rhs) {
			return lhs.weight < rhs.weight;
		});
	}

	//our shortcut to get out of the N-round loop if we converge early (we will)

  for (int round=0;round<10;round++){
	
		cerr << "Round "<<round<<" done, " << comms.bytes_sent()<< " bytes sent with "<<comms.msgs_sent()<<" msgs"<<endl;

		cerr << "Leaders"<<endl;
		for (size_t i=0;i< num_agents;i++){
			cerr<<" "<<i<<"->"<<part_leader[i];
		}
		cerr<<endl;

		cerr << "Parents"<<endl;
		for (size_t i=0;i< num_agents;i++){
			cerr<<" "<<i<<"->"<<parent[i];
		}
		cerr<<endl;

		cerr << "Waiting (should be 0)"<<endl;
		for (size_t i=0;i< num_agents;i++){
			cerr<<" "<<i<<"->"<<waiting_for[i];
		}
		cerr<<endl;



		cerr << "Graph: (C=child_of; P=parent_of; .=self-loop; '-'=discarded; x=error/other;"<<endl;

		cerr << "   ";

		size_t width = 1+floor(log(num_agents));
		for (size_t i=0;i<num_agents;i++){
			cerr.width(width);
			cerr << i;
		}

		cerr << endl;
		for (size_t i=0;i<num_agents;i++){
			cerr.width(width);
			cerr << i; 
			for (size_t j=0;j<num_agents;j++){
				char to_print;
				if (i==j) to_print='.';
				else if (parent[j]==i) to_print='P';
				else 
					switch (edge_class[i][j]){
						case MST:{
							to_print='C';
							break;
						}
						case DISCARDED:{
							to_print='-';
							break;
						}
						case UNKNOWN_EDGE:{
							to_print = '?';
							break;
						}
						default :{
							to_print='x';
							break;
						}
					}

				cerr.width(width);
				cerr<<to_print;
			}
			cerr<<endl;

		}

	}

	//write out the MST:
	for (size_t i=0;i<num_agents;i++){
		size_t m_parent = parent[i];
		size_t weight = connectivity_matrix[i][m_parent];
		char designation='T';
		if (m_parent==i){
			designation='R';
		}
		std::cout<<i<<" "<<m_parent<<" "<<weight<<" "<<designation<<endl;

	}


    std::cerr<<"ghs-score done"<<std::endl;
	return 0;
}
