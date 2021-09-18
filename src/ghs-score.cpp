#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <assert.h>
#include <deque>

/**
 * We're missing a few optimizations:
 * # Optimization 1:
 * We can test outgoing edges (IN_PART) one at a time, instead of all at once.
 * To do this, set waiting == # of edges that are unknown, then send in_part
 * msgs from smallest weight to largest. In the checK_search() routine, if you
 * receive a NACK_PART message, you can rest assured its along the lowest
 * outgoing edge. If you receive a ACK_PART, then you need to try the next edge
 * 
 * Optimization 2:
 * Leader election can be optimized so it just runs on one of the two nodes
 * adjoining the MWOE that was just used to connect two components. I don't
 * fully undersatnd how (ch 15 Lynch)
 * 
 */

#define MAX_AGENTS 1024
using std::cout;
using std::cerr;
using std::endl;
using std::cin;

typedef struct
{
	enum Type
	{
		SRCH = 0,
		SRCH_RET, //MWOE
		IN_PART,  //part_id
		ACK_PART,
		NACK_PART,
		JOIN_US,
		ELECTION,
		NOT_IT,
		NEW_SHERIFF
	} type;
	int to, from;
	std::vector<int> data;
} Msg;

//system stuff / simulate comms
namespace comms
{
	std::deque<Msg> msg_q;
	int amt_sent(0);
	void send(Msg::Type type, int to, int from, std::vector<int> data = {})
	{
		msg_q.emplace_back(Msg{type, to, from, data});
		amt_sent += data.size() + 2;
	}
	int waiting() { return msg_q.size(); }
	Msg get()
	{
		Msg r = msg_q.front();
		msg_q.pop_front();
		return r;
	}
}

struct Edge
{
	int to, from;
	int weight;
};

enum EdgeClass
{
	UNKNOWN_EDGE = 0,
	DISCARDED = -1,
	MST = 1
};

struct Nodes{
	Nodes(int n)
	{
		num_agents=n;
		connectivity_matrix = std::vector<std::vector<double>>(num_agents, std::vector<double>(num_agents, 0.0));
		edge_class = std::vector<std::vector<int>>(num_agents, std::vector<int>(num_agents, UNKNOWN_EDGE));
		parent = std::vector<int>(num_agents);
		part_leader = std::vector<int>(num_agents);
		waiting_for = std::vector<int>(num_agents);
		best_edge = std::vector<Edge>(num_agents);
	}

	bool is_root(int nid){
		return parent[nid]==nid;
	}

	int num_agents;
	//Here's the node memories. Each i^th row is the memory for the i^th node
	std::vector<std::vector<double>> connectivity_matrix;
	//what do I know about my neighbors comms links?
	std::vector<std::vector<int>> edge_class;
	//who do I talk through to the leader?
	std::vector<int> parent;
	//who's in charge around here
	std::vector<int> part_leader;
	//how many IN_PART msgs are we waiting for?
	std::vector<int> waiting_for;
	std::vector<Edge> best_edge;
};


void check_search(int nid, Nodes& nodes)
{
	//while pretending to be nid,
	//are we waiting for any incoming ACK/NACK msgs? 
	assert(nodes.waiting_for[nid]>=0); //ask me why I know this is a fail case
	cout<<"Node: "<< nid << " waiting for: "<<nodes.waiting_for[nid]<<endl;
	if (nodes.waiting_for[nid] == 0)
	{
		//nope not waiting
		cout << "Node: " << nid << " got all respones" << endl; 
		auto best_edge = nodes.best_edge[nid];
		auto parent_link = nodes.parent[nid];
		if ( ! nodes.is_root(nid))
		{
			cout<<"Node: "<< nid <<" is NOT root, sending data back to "<<parent_link<<endl;
			comms::send(Msg::Type::SRCH_RET, parent_link, nid, {best_edge.to, best_edge.from, best_edge.weight});
		}
		else if (best_edge.from == nid){
			int node_to_add = best_edge.to;
			cout<<"Node: "<< nid <<" is root and leaf! Adding edge."<<endl;
			nodes.edge_class[nid][node_to_add]=MST;
			comms::send(Msg::Type::JOIN_US,node_to_add,nid, {best_edge.to,best_edge.from,best_edge.weight});
		} else {
			cout<<"Node: "<< nid <<" is root and not leaf, broadcasting edge choice"<<endl;
			for (int i =0;i<nodes.num_agents;i++){
				if (nodes.edge_class[nid][i]==MST){
					comms::send(Msg::Type::JOIN_US,i,nid, {best_edge.to,best_edge.from,best_edge.weight});
				}
			}
		}
	}
}

int main(int argc, char *argv[]) 
{
	
	Nodes nodes(10);

	for (int i = 0; i < nodes.num_agents; i++)
	{
		//which partition is each node part of?
		nodes.part_leader[i] = i; //own partition so far:w
		//who is parent in the MST?
		nodes.parent[i] = i; //i_am_root!
	}

	for (int i = 0; i < nodes.num_agents; i++)
	{
		comms::send(Msg::Type::SRCH, i, i, {});
	}
	while(comms::waiting() > 0)
	{
		auto m = comms::get();
		int nid = m.to; //"acting" as this node
		int sender = m.from;
		switch (m.type)
		{
			case (Msg::Type::SRCH):
			{
				nodes.best_edge[nid] = Edge{0, 0, std::numeric_limits<int>::max()};
				for (int i = 0; i < nodes.num_agents; i++)
				{
					if (i == nid) { continue; }
					switch (nodes.edge_class[nid][i])
					{
						//not sure what's going on with the guy on the other end. Maybe
						//this is the new Minimum Outgoing Edge?
						case UNKNOWN_EDGE:
						{
							nodes.waiting_for[nid]++;
							//let's ask if he's in our partition
							comms::send(Msg::Type::IN_PART, i, nid, {nodes.part_leader[m.to]});
							break;
						}

						//I know this edge leads down the MST for my partition
						case MST:
						{
							nodes.waiting_for[nid]++;
							//propegate the search forward
							comms::send(Msg::Type::SRCH, i, nid);
							break;
						}
					}
				}
				break;
			}

			case Msg::Type::IN_PART:
			{
				auto qpart = m.data[0];
				cout << "Node: " << nid << " received from " << sender << " 'IN_PART " << qpart << "?'" << endl;
				if (qpart == nodes.part_leader[nid])
				{
					comms::send(Msg::Type::ACK_PART, sender, nid);
				}
				else
				{
					comms::send(Msg::Type::NACK_PART, sender, nid);
				}
				break;
			}

			case Msg::Type::ACK_PART:
			{
				cout << "Node: " << nid << " received from " << sender << ", discarding edge" << endl;
				nodes.edge_class[nid][sender] = DISCARDED;
				nodes.waiting_for[nid]--;
				check_search(nid,nodes);
				break;
			}

			case Msg::Type::NACK_PART:
			{
				cout << "Node: " << nid << " received from " << sender << " NACK_PART" << endl;
				nodes.waiting_for[nid]--;
				int to=nodes.best_edge[nid].to;
				int from=nodes.best_edge[nid].from;
				int best_weight=nodes.best_edge[nid].weight;
				int proposed_weight = nodes.connectivity_matrix[nid][sender];

				if (best_weight > proposed_weight){
					nodes.best_edge[nid].to=sender;
					nodes.best_edge[nid].from=nid;
					nodes.best_edge[nid].weight=proposed_weight;
				}

				check_search(nid,nodes);
				break;
			}

			case Msg::Type::JOIN_US:
			{
				cout << "Node: " << nid << " received from " << sender << " JOIN_US (" << m.data[0]<<","<<m.data[1]<<")"<<endl;
			}

		}// switch m.type
	}// while comms::waiting()
	cerr << "Done, " << comms::amt_sent << " sent.";
	return 0;
}
