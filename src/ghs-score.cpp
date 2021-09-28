#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <random>
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
 * unknown. Sending in-Part msgs from smallest weight to largest. 
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
 * And we have some assumptions that might be questionable:
 * 
 * Assumption 1: Comms is symmetric between nodes (if I can hear you, you can hear me just as well)
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

enum EdgeClass
{
	UNKNOWN_EDGE = 0,
	DISCARDED = -1,
	MST = 1
};

struct Nodes{
	Nodes(int n)
	{
		//world knowledge
		num_agents=n; //also known by agents
		connectivity_matrix = std::vector<std::vector<double>>(num_agents, std::vector<double>(num_agents, 0.0));

		//local communication / topology information known to nodes
		edge_class = std::vector<std::vector<EdgeClass>>(num_agents, std::vector<EdgeClass>(num_agents, UNKNOWN_EDGE));
		parent = std::vector<int>(num_agents);
		part_leader = std::vector<int>(num_agents,0);
		part_level = std::vector<int>(num_agents,0);
		edge_list = std::vector<std::vector<Edge>>(num_agents,std::vector<Edge>(num_agents,{0,0,std::numeric_limits<int>::max()}));

		//local algorithm step memory
		waiting_for = std::vector<int>(num_agents,0);
		best_edge = std::vector<Edge>(num_agents);
	}

	int num_agents;
	//Here's the node memories. Each i^th row is the memory for the i^th node
	std::vector<std::vector<double>> connectivity_matrix;
	std::vector<std::vector<Edge>> edge_list;
	//what do I know about my neighbors comms links?
	std::vector<std::vector<EdgeClass>> edge_class;
	//who do I talk through to the leader?
	std::vector<int> parent;
	//who's in charge around here
	std::vector<int> part_leader;
	//what "level" is my partition?
	std::vector<int> part_level;
	//how many IN_PART msgs are we waiting for?
	std::vector<int> waiting_for;
	//what's the best edge I've seen?
	std::vector<Edge> best_edge;
};


int main(int argc, char *argv[]) 
{
	
	Nodes nodes(10);

	srand(99);

	for (int i = 0; i < nodes.num_agents; i++)
	{
		//which partition is each node part of?
		nodes.part_leader[i] = i; //own partition so far
		//who is parent in the MST?
		nodes.parent[i] = i; //i_am_root!
 
		for (int j=i+1;j<nodes.num_agents;j++){

			//int r = 1;
			int r = rand() % 100 + 1; 
			nodes.connectivity_matrix[i][j] = r;
			nodes.connectivity_matrix[j][i] = r;
			Edge e{i,j,r};
			Edge e2{j,i,r};
			nodes.edge_list[i][j]=e;
			nodes.edge_list[j][i]=e2;
		}
		std::sort(std::begin(nodes.edge_list[i]), std::end(nodes.edge_list[i]),
				[] (const Edge& lhs, const Edge& rhs) {
			return lhs.weight < rhs.weight;
		});
	}

	bool converged = false;
	//for now, hard-code N rounds (far, far too many, but whatever)
	main_loop: 
	for (int round=0;round<nodes.num_agents && !converged;round++){

		for (int i=0;i<nodes.num_agents;i++){
			if (nodes.waiting_for[i] > 0){
				cerr << "=====WARNING======"<<endl;
				cerr << "Node: "<<i<< " was waiting for msgs from last roung"<<endl;

			}
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

		while(comms::waiting() )
		{

			auto m = comms::get();
			int nid = m.to; //"acting" as this node
			int sender = m.from;

			//some implementation sanity checks for this node:
			assert(nodes.waiting_for[nid]>=0);
			assert(nodes.waiting_for[nid]<=nodes.num_agents);

			switch (m.type)
			{
				case (Msg::Type::SRCH):
				{
					//this is a new search request (we'll receive only one search, since it goes out over MST links or from self)
					nodes.best_edge[nid] = Edge{0, 0, std::numeric_limits<int>::max()};

					//check each outgoing link
					for (int i = 0; i < nodes.num_agents; i++)
					{

						//two special kinds of links:

						//1. don't sent to myself
						if (i == nid) { continue; }
						//2. don't send "up" the tree
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
								//still need a response from them
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

					//NOTE: If we do not send any messages in this block, then
					//we are sure there are no MWOE attached to our subtree. 
					// we need to let someone know! That someone is the parent.
					if (nodes.waiting_for[nid]==0)
					{
						//special case, no minimum weight outgoing edge
						cerr << " node "<<nid<<" has no edges to search!"<<endl;
						auto best_edge = nodes.best_edge[nid];
						auto parent_link = nodes.parent[nid];// WARNING THIS COULD BE US! See SRCH_RET!
						cerr << "Node: " << nid << " sending data back to "<<parent_link<<endl;
						comms::send(Msg::Type::SRCH_RET, parent_link, nid, {best_edge.to, best_edge.from, best_edge.weight});
					}

					break;//end processing SRCH
				}

				case Msg::Type::IN_PART: // are you in my partition? Yes (ACK) or no (NACK)
				{
					auto qpart = m.data[0];
					if (qpart == nodes.part_leader[nid])
					{
						//it seems we are in their partition
						cerr << " node "<<nid<<" is part of "<<qpart<<endl;
						comms::send(Msg::Type::ACK_PART, sender, nid, {qpart});
					}
					else
					{
						//it seems we are not
						cerr << " node "<<nid<<" is NOT part of "<<qpart<<endl;
						comms::send(Msg::Type::NACK_PART, sender, nid, {qpart});
					}
					break;
				} // end IN_PART

				case Msg::Type::ACK_PART:
				{

					//any nodes who reply with ACK_PART are already in my partition. Let's ignore them going forward.
					nodes.edge_class[nid][sender] = DISCARDED;
					nodes.waiting_for[nid]--;
					cerr << " node "<<nid<<" now waiting for "<<nodes.waiting_for[nid]<<endl;

					//was that the last response we needed?
					if (nodes.waiting_for[nid]==0){

						//if so, let's answer our parent's request for MWOE
						auto best_edge = nodes.best_edge[nid];
						auto parent_link = nodes.parent[nid];
						cerr << "Node: " << nid << " got all respones sending data back to "<<parent_link<<endl;
						comms::send(Msg::Type::SRCH_RET, parent_link, nid, {best_edge.to, best_edge.from, best_edge.weight});
					}
					break;
				} // end ACK_PART

				case Msg::Type::NACK_PART:
				{
					//oh they were NOT in our partition? OK, let's record that helpful information.
					nodes.waiting_for[nid]--;
					cerr << " node "<<nid<<" now waiting for "<<nodes.waiting_for[nid]<<endl;

					//save the edge from me to the sender
					int to=nodes.best_edge[nid].to;
					int from=nodes.best_edge[nid].from;
					int best_weight=nodes.best_edge[nid].weight;
					int proposed_weight = nodes.connectivity_matrix[nid][sender];

					//but really only if it's better than the best one
					if (best_weight > proposed_weight){
						nodes.best_edge[nid].to=sender;
						nodes.best_edge[nid].from=nid;
						nodes.best_edge[nid].weight=proposed_weight;
					}

					//Are we done looking now? 
					if (nodes.waiting_for[nid]==0){
						//yes! Let's propegate up the best found outoing edge
						auto best_edge = nodes.best_edge[nid];
						auto parent_link = nodes.parent[nid];
						cerr << "Node: " << nid << " got all respones sending data back to "<<parent_link<<endl;
						comms::send(Msg::Type::SRCH_RET, parent_link, nid, {best_edge.to, best_edge.from, best_edge.weight});
					}

					//NOTE: There's one special case of parent==nid We dont'
					//want to handle all the logic of aggregating the search
					//results in each type of respones (ACK,NACK, etc), so we
					//instead bundle up the result and send to parent *even if
					//that is ourself* to trigger the SRCH_RET processing in the
					//next block. This greatly simplifies the codebase, but does
					//cause some edge conditions in the next block
					break;
				} // end NACK_PART

				case Msg::Type::SRCH_RET: 
				{

					cerr << " node " << nid << " waiting for " << nodes.waiting_for[nid] << endl;
					//we got an answer to all our SRCH msgs from the downstream nodes who collected ACK_PART and NACK_PART respones
					//NOTE: This could be a resonse from *ourselves*, so be wary of that!
					if (sender != nid)
					{
						//ok, this is from someone else, not just from the previous "stage" of the algorithm
						nodes.waiting_for[nid]--;
						cerr << " node " << nid << " now waiting for " << nodes.waiting_for[nid] << endl;
						auto best_edge = nodes.best_edge[nid];
						//let's store their answer, it could be better than ours.

						auto candidate_edge = Edge{m.data[0], m.data[1], m.data[2]};
						if (candidate_edge.weight < best_edge.weight)
						{
							best_edge = candidate_edge;
							nodes.best_edge[nid] = candidate_edge;
						}
					}

					//now, are we still waiting to communicate to parent?

					//load up the memory for node 'nid'
					auto parent_link = nodes.parent[nid];
					auto best_edge = nodes.best_edge[nid];
					if (nodes.waiting_for[nid]==0){

						//special case, am I root?
						if (nodes.part_leader[nid]==nid){
							cerr << "Node: "<< nid << " is leader, broadcasting new join_us .. maybe"<<endl;
							if (best_edge.weight ==  std::numeric_limits<int>::max()){
								cerr << "Node: "<< nid << " is leader, but found no suitable edge, algorithm can terminate! "<<endl;
								//rust, I pine for your oxidization:
								//break main_loop; 	
								converged=true;
							}
							else
							{
								//if we found a new edge let's broadcast the new best edge according to my glorious wishes!
								for (int i = 0; i < nodes.num_agents; i++)
								{
									//hear ye MST!
									if ( i==nid || nodes.edge_class[nid][i] == MST /* && nid!=i The leader should follow join_us commands from itself*/)
									{
										//Let all behold your leader -- who is me -- and tremble
										int leader = nid;
										//send forth my commands for node "i" to join the faithful by adding edge 'best_edge' to our MST
										comms::send(Msg::Type::JOIN_US, i, nid, {best_edge.to, best_edge.from, best_edge.weight, leader});
									}
								}
							}
						} else {
							//I'm not leader, but we definitely got all our responses
							cerr << "Node: " << nid << " got all respones sending data back to "<<parent_link<<endl;
							comms::send(Msg::Type::SRCH_RET, parent_link, nid, {best_edge.to, best_edge.from, best_edge.weight});
						}
					}
					break; 
				} // end processing SRCH_RET

				case Msg::Type::JOIN_US:
				{


					//TODO (Optimization 2, above)
					//if we already sent a JOIN_US to them, then one of us is the leader

					//ok, a brand new edge is being added, sent along the MST links
					//we're been told to join the edge to the MST:
					int to = m.data[0];
					int from = m.data[1];
					int cand_leader = m.data[3];

					if (from != nid && to !=nid){
						//this command does not apply to us
						//not much for us to do but propegate
						for (int i=0;i<nodes.num_agents;i++){
							if (nodes.edge_class[nid][i]== MST && i!=sender){
								cerr<<" propegating JOIN_US to "<<i<<endl;
								comms::send(m.type, i, nid, m.data);
							}
						}
						break;
					}

					//ok we're one of the important nodes involved in the join
					//first let's mark the edge appropriately
					if (nid == to){
						//edge is incoming, they will ping us when they add and
						//we'll sort out who is parent with NEW_SHERIFF msgs
						nodes.edge_class[nid][from]=MST;
					} else if (nid == from){
						//edge is outgoing
						nodes.edge_class[nid][to]=MST;
						//tell them about the new boss (my boss)
						int my_leader = nodes.part_leader[nid];
						int wt_for_ref = nodes.connectivity_matrix[sender][nid];
						comms::send(Msg::Type::JOIN_US, to, nid, {to /*them*/, from /*me*/, wt_for_ref, my_leader});
						comms::send(Msg::Type::NEW_SHERIFF, to, nid, {my_leader});
						//NEW_SHERIFF gives them a chance to rebutt with a
						//better choice, see below
					}


					break;
				} // end JOIN_US

				case Msg::Type::NEW_SHERIFF:
				{

					if (nodes.edge_class[nid][sender]!=MST){
						cerr << "Node "<< nid << " did not recognize sender ("<<sender<<") as MST link!"<<endl;
					}
					if (nodes.edge_class[sender][nid]!=MST){
						cerr << "ERRR: Node "<< sender << " did not recognize " << nid << " as MST link but still sent NEW_SHERIFF msg"<<endl;
					}
					//assert(nodes.edge_class[sender][nid]==MST);
					//assert(nodes.edge_class[nid][sender]==MST);
					//OK, we got the links in, but now we're arguing about who
					//is boss of the new graph, which affects everything from
					//who makes decisions to the polarity of MST links /
					//parenthood.
					//
					//Since this action originates at the site of an add ... 
					//TODO: Leader selection should take place only at leaders or recently-joined nodes
					int cand_leader = m.data[0];
					if (nodes.part_leader[nid]==cand_leader){
						cerr << "Node: "<<nid<<" already joined "<<m.data[0];
						//we don't need to propegate, these messages originate
						//not from roots, but from the boundary between
						//partitions, so they'll only go until they hit the same
						//partition, as evidenced by leaders
					}
					else if (cand_leader>nodes.part_leader[nid]){
						//hot damn they do have a nice leader, let's join them!
						cerr <<" Node: "<<nid<<" joining new leader: "<<m.data[0]<<endl;
						nodes.part_leader[nid]=cand_leader;
						nodes.parent[nid]=sender;

						//in which case, we should propegate
						for (int i =0;i<nodes.num_agents;i++){
							if (i!=sender && nodes.edge_class[nid][i]==MST){
								comms::send(Msg::Type::NEW_SHERIFF, i,nid, {cand_leader});
							}
						}
					} else {
						//no need to change anything, their leader option is worse, but 
						//let's tell them that so they can fall in line
						cerr <<" Node: "<<nid<<" sending new leader instead of joining: "<<m.data[0]<<endl;
						int my_leader = nodes.part_leader[nid];
						comms::send(Msg::Type::NEW_SHERIFF, sender,nid, {my_leader});
					}
					break;
				} // end NEW_SHERIFF

				default :{
					cerr<<"=====WARNING UNHANDLED MESSAGE=====";
					cerr<<"Type: "<<m.type<<endl;
					cerr<<"From: "<<sender<<endl;
					cerr<<"To:   "<<nid<<endl;
				} // end default

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