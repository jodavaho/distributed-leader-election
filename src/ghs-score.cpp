#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <assert.h>
#include <fstream>
#include <math.h>
#include <deque>

/**
 * We're missing a few optimizations:
 * 
 * # Optimization 1:
 * We can test outgoing edges (IN_PART) one at a time, instead of all at once.
 * To do this, set waiting == 0, and use it to count up to # of edges that are
 * unknown. Sending in-Part msgs from smallest weight to largest. In the
 * check_search() routine, if you receive a NACK_PART message, you can rest
 * assured its along the lowest outgoing edge. If you receive a ACK_PART, then
 * you need to try the next edge
 * 
 * Optimization 2:
 * Leader election can be optimized so it just runs on one of the two nodes
 * adjoining the MWOE that was just used to connect two components. I don't
 * fully undersatnd how (ch 5/15 Lynch). Firguring that out later.
 * 
 * Optimization 3:
 * WHen receiving IN_PART X, if you are in X, then you can probably discard the
 * edge, as long as you ignore the sending node when you are waiting_for IN_PART
 * msgs that *you* sent
 * 
 */

#define MAX_AGENTS 1024
using std::cerr;
using std::cerr;
using std::endl;
using std::cin;

typedef struct
{
	enum Type
	{
		NOOP = 0,
		SRCH,
		SRCH_RET, 
		IN_PART,  
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

std::string to_string(const Msg::Type &type){
	switch (type){
		case Msg::Type::SRCH:{return "SRCH";}
		case Msg::Type::SRCH_RET:{return "SRCH_RET";}
		case Msg::Type::IN_PART:{return "IN_PART";}
		case Msg::Type::ACK_PART:{return "ACK_PART";}
		case Msg::Type::NACK_PART:{return "NACK_PART";}
		case Msg::Type::JOIN_US:{return "JOIN_US";}
		case Msg::Type::ELECTION:{return "ELECTION";}
		case Msg::Type::NOT_IT:{return "NOT_IT";}
		case Msg::Type::NEW_SHERIFF:{return "NEW_SHERIFF";}
		default: {return "??";};
	}
}

std::ostream& operator << ( std::ostream& outs, const Msg::Type & type )
{
  return outs << to_string(type);
}


std::ostream& operator << ( std::ostream& outs, const Msg & m)
{
  outs << "("<<m.from<<"-->"<< m.to<<") "<< m.type<<" {";
  for (const auto data: m.data){
	  outs<<" "<<data;
  }
  outs<<" }";
  return outs;
}

//system stuff / simulate comms
namespace comms
{
	std::deque<Msg> msg_q;
	int msgs_sent(0);
	int amt_sent(0);
	void send(Msg::Type type, int to, int from, std::vector<int> data = {})
	{
		std::ofstream ofile("msgs.csv",std::fstream::out | std::fstream::app);
		Msg m {type, to, from, data};
		ofile<<m<<endl;
		cerr<<"(++"<<msg_q.size()<<")"<<"Sending: "<<m<<endl;
		msg_q.emplace_back(m);
		msgs_sent++;
		amt_sent += data.size() + 2;
	}
	int waiting() { return msg_q.size(); }
	Msg get()
	{
		Msg r = msg_q.front();
		cerr<<"(--"<<msg_q.size()<<")"<<"Processing: "<<r<<endl;
		msg_q.pop_front();
		return r;
	}
}

struct Edge
{
	int to, from;
	int weight;
};

bool operator==(const Edge& lhs, const Edge& rhs){
	return (lhs.to == rhs.to) && (lhs.from==rhs.from);
}

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
		edge_class = std::vector<std::vector<EdgeClass>>(num_agents, std::vector<EdgeClass>(num_agents, UNKNOWN_EDGE));
		parent = std::vector<int>(num_agents);
		part_leader = std::vector<int>(num_agents);
		waiting_for = std::vector<int>(num_agents);
		best_edge = std::vector<Edge>(num_agents);
	}

	int num_agents;
	//Here's the node memories. Each i^th row is the memory for the i^th node
	std::vector<std::vector<double>> connectivity_matrix;
	//what do I know about my neighbors comms links?
	std::vector<std::vector<EdgeClass>> edge_class;
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
	if (nodes.waiting_for[nid] == 0)
	{
		auto best_edge = nodes.best_edge[nid];
		//nope not waiting
		cerr << "Node: " << nid << " got all respones" << endl; 
		auto parent_link = nodes.parent[nid];

		if (  nodes.part_leader[nid]!=nid ) 
		{
			cerr<<"Node: "<< nid <<" is NOT root, sending data back to "<<parent_link<<endl;
			comms::send(Msg::Type::SRCH_RET, parent_link, nid, {best_edge.to, best_edge.from, best_edge.weight});
		}

		else if (best_edge.from == nid){
		//we know we're root, but what if we're also the node on the edge to add?
			int node_to_add = best_edge.to;

			if (best_edge.weight == std::numeric_limits<int>::max()){
				//actually, we didn't find one, everyone sent back (-1,-1)
				cerr<<"No MWOE found, terminating"<<endl;
				return;
			} 
			
			else {
				cerr<<"Node: "<< nid <<" is root and leaf! Adding edge from "<<nid<<" to "<<node_to_add<<endl;
				nodes.edge_class[nid][node_to_add]=MST;
				int my_leader = nodes.part_leader[nid];
				comms::send(Msg::Type::JOIN_US,node_to_add,nid, {best_edge.to,best_edge.from,best_edge.weight,my_leader});
			}

		} 
		
		else {
			//now we're an internal root (not leaf, yes leader)
			if (best_edge.weight==std::numeric_limits<int>::max()){
				cerr<<"Node: "<< nid <<" is root and found no MWOE"<<endl;
				return;
			}

			else {
				cerr << "Node: " << nid << " is root and not leaf, broadcasting edge choice" << endl;

				for (int i = 0; i < nodes.num_agents; i++)
				{
					if (nodes.edge_class[nid][i] == MST)
					{
						int my_leader = nodes.part_leader[nid];
						comms::send(Msg::Type::JOIN_US, i, nid, {best_edge.to, best_edge.from, best_edge.weight, my_leader});
					}
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
		nodes.part_leader[i] = i; //own partition so far
		//who is parent in the MST?
		nodes.parent[i] = i; //i_am_root!
	}


	//for now, hard-code N rounds (far, far too many, but whatever)
	for (int round=0;round<nodes.num_agents;round++){

		for (int i=0;i<nodes.num_agents;i++){
			nodes.waiting_for[i]==0;
		}

		cerr<<"Round "<<round<<"========"<<endl;
		//begin rounds by sending out search requests from the roots
		for (int i = 0; i < nodes.num_agents; i++)
		{
			if (nodes.part_leader[i]==i){

				cerr << "Leader: "<<i<< " starting search "<<endl;
				comms::send(Msg::Type::SRCH, i, i, {});
			}
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
						//don't sent to myself
						if (i == nid) { continue; }
						//don't send "up" the tree
						if (i == nodes.parent[nid]){ continue; }
						switch (nodes.edge_class[nid][i])
						{

							//not sure what's going on with the guy on the other end. Maybe
							//this is the new Minimum Outgoing Edge?
							//See optimization 1, above
							case UNKNOWN_EDGE:
							{
								nodes.waiting_for[nid]++;
								cerr << "Unknown edge found to "<<i<<endl;
								cerr << " node "<<nid<<" now waiting for "<<nodes.waiting_for[nid]<<endl;
								//let's ask if he's in our partition
								comms::send(Msg::Type::IN_PART, i, nid, {nodes.part_leader[nid]});
								break;
							}

							//I know this edge leads down the MST for my partition
							case MST:
							{
								nodes.waiting_for[nid]++;
								cerr << "MST edge found to "<<i<<endl;
								cerr << " node "<<nid<<" now waiting for "<<nodes.waiting_for[nid]<<endl;
								//propegate the search forward
								comms::send(Msg::Type::SRCH, i, nid);
								break;
							}

							//otherwise, we don't need to bother the other nodes. 
							defaut:{
								//nothing
								break;
							}
						}
					}
					if (nodes.waiting_for[nid]==0)
					{
						//special case, no minimum weight outgoing edge

					}
					break;
				}

				case Msg::Type::SRCH_RET:
				{
					nodes.waiting_for[nid]--;
					cerr << " node "<<nid<<" now waiting for "<<nodes.waiting_for[nid]<<endl;
					//we got a return from our children!
					check_search(nid,nodes);
					break;

				}

				case Msg::Type::IN_PART:
				{
					auto qpart = m.data[0];
					if (qpart == nodes.part_leader[nid])
					{
						cerr << " node "<<nid<<" is part of "<<qpart<<endl;
						comms::send(Msg::Type::ACK_PART, sender, nid, {qpart});
					}
					else
					{
						cerr << " node "<<nid<<" is NOT part of "<<qpart<<endl;
						comms::send(Msg::Type::NACK_PART, sender, nid, {qpart});
					}
					break;
				}

				case Msg::Type::ACK_PART:
				{
					nodes.edge_class[nid][sender] = DISCARDED;
					nodes.waiting_for[nid]--;
					cerr << " node "<<nid<<" now waiting for "<<nodes.waiting_for[nid]<<endl;
					check_search(nid,nodes);
					break;
				}

				case Msg::Type::NACK_PART:
				{
					nodes.waiting_for[nid]--;
					cerr << " node "<<nid<<" now waiting for "<<nodes.waiting_for[nid]<<endl;
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
					int cand_leader = m.data[3];

					//TODO (Optimization 2, above)
					//if we already sent a JOIN_US to them, then one of us is the leader

					//can't just add MST link yet, need to propegate messages to our partition
					if (cand_leader>nodes.part_leader[nid]){
						cerr<<"Candidate leader: "<<cand_leader<<" wins"<<endl;
						//new boss in town, send out the word to our old boss
						for (int i=0;i<nodes.num_agents;i++){
							if (nodes.edge_class[nid][i]== MST){
								cerr<<" notifying of new leader: "<<i<<endl;
								comms::send(Msg::Type::NEW_SHERIFF, i, nid, {cand_leader});
							}
						}
						// ... and fall in line
						nodes.part_leader[nid]=cand_leader;
						nodes.parent[nid]=sender;
					} 
					//if the new leader isn't 'better', then we don't need to notify anyone

					//at any rate, add this as an MST link
					nodes.edge_class[nid][sender]=MST;
					break;
				}

				case Msg::Type::NEW_SHERIFF:
				{
					int cand_leader = m.data[0];
					if (nodes.part_leader[nid]==cand_leader){
						cerr << "Node: "<<nid<<" already joined "<<m.data[0];
					}
					else if (cand_leader>nodes.part_leader[nid]){
						cerr <<" Node: "<<nid<<" joining new leader: "<<m.data[0]<<endl;
						//new to me!
						nodes.part_leader[nid]=cand_leader;
						nodes.parent[nid]=sender;

						//in which case, we should propegate
						for (int i =0;i<nodes.num_agents;i++){
							if (i!=sender && nodes.edge_class[nid][i]==MST){
								comms::send(Msg::Type::NEW_SHERIFF, i,nid, {cand_leader});
							}
						}
					}
					break;
				}

				default :{
					cerr<<"=====WARNING UNHANDLED MESSAGE=====";
					cerr<<"Type: "<<m.type<<endl;
					cerr<<"From: "<<sender<<endl;
					cerr<<"To:   "<<nid<<endl;
				}

			}/* switch m.type*/
		}/* while comms::waiting()*/

		cerr << "Round "<<round<<" done, " << comms::amt_sent << " bytes sent with "<<comms::msgs_sent<<" msgs"<<endl;

		cerr << "Leaders"<<endl;
		for (int i=0;i< nodes.num_agents;i++){
			cerr<<" "<<i<<"->"<<nodes.part_leader[i];
		}
		cerr<<endl;

		cerr << "Parents"<<endl;
		for (int i=0;i< nodes.num_agents;i++){
			cerr<<" "<<i<<"->"<<nodes.parent[i];
		}
		cerr<<endl;

		cerr << "Waiting (should be 0)"<<endl;
		for (int i=0;i< nodes.num_agents;i++){
			cerr<<" "<<i<<"->"<<nodes.waiting_for[i];
		}
		cerr<<endl;


		cerr << "Graph: (C=child_of; P=parent_of; .=self-loop; '-'=discarded; x=error/other;"<<endl;

		cerr << "   ";

		int width = 1+floor(log(nodes.num_agents));
		for (int i=0;i<nodes.num_agents;i++){
			cerr.width(width);
			cerr << i;
		}

		cerr << endl;
		for (int i=0;i<nodes.num_agents;i++){
			cerr.width(width);
			cerr << i; 
			for (int j=0;j<nodes.num_agents;j++){
				char to_print;
				if (i==j) to_print='.';
				else if (nodes.parent[j]==i) to_print='P';
				else 
					switch (nodes.edge_class[i][j]){
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
	return 0;
}