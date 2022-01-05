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

/**
 * We're missing a few optimizations:
 * 
 * # Optimization 1:
 * We can test outgoing edges (IN_PART) one at a time, instead of all at once.
 * To do this, set waiting == 0, and use it to count up to # of edges that are
 * unknown. Sending in-Part msgs from smallest weight to largest.  That way, the
 * first NACK_PART message you receive is your lowest weight edge. 
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
 * Assumption 1: Comms is symmetric between nodes (if I can hear you, you can
 * hear me just as well)
 * 
 */

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
	bool converged = false;
	

	for (size_t round=0;round<num_agents && !converged;round++){

		for (size_t i=0;i<num_agents;i++){
			if (waiting_for[i] > 0){
				//this is an annyoing failure case for large N: we're delcaring
				//victory for a round before we actually hear from all other
				// I'm working on it. 
				cerr << "=====WARNING======"<<endl;
				cerr << "Node: "<<i<< " was waiting for msgs from last roung"<<endl;

			}
		}

		cerr<<"Round "<<round<<"========"<<endl;
		//begin rounds by sending out search requests from the roots this
		//effectively "starts" a round. 
		for (size_t i = 0; i < num_agents; i++)
		{
			if (part_leader[i]==i){

				cerr << "Leader: "<<i<< " starting search "<<endl;
				comms.send(Msg::Type::SRCH, i, i, {});
			}
		}

		//process messages while they exist in side this loop. 
		while(comms.waiting() )
		{

			auto m = comms.get();
			size_t nid = m.to; //"acting" as this node
			size_t sender = m.from;

			//some implementation sanity checks for this node. This doesn't
			//catch those weird cases where a node just doesn't hear back and
			//waiting_for monotinically icnreases. 
			assert(waiting_for[nid]>=0);
			assert(waiting_for[nid]<=num_agents);

			switch (m.type)
			{
				case (Msg::Type::SRCH):
				{
					//this is a new search request (we'll receive only one search, since it goes out over MST links or from self)
					best_edge[nid] = Edge{0, 0, std::numeric_limits<size_t>::max()};

					//Note that we wait 
					waiting_for[nid]=0;

					//check each outgoing link
					for (size_t i = 0; i < num_agents; i++)
					{

						//two special kinds of links:

						//1. don't sent to myself
						if (i == nid) { continue; }
						//2. don't send "up" the tree
						if (i == parent[nid]){ continue; }

						switch (edge_class[nid][i])
						{
							//not sure what's going on with the guy on the other end. Maybe
							//this is the new Minimum Outgoing Edge?
							//See optimization 1, above
							case UNKNOWN_EDGE:
							{
								assert(nid!=i);
								waiting_for[nid]++;
								cerr << "Unknown edge found to "<<i<<endl;
								cerr << " node "<<nid<<" now waiting for "<<waiting_for[nid]<<endl;
								//let's ask if he's in our partition
								comms.send(Msg::Type::IN_PART, i, nid, {part_leader[nid]});
								break;
							}

							//I know this edge leads down the MST for my partition
							case MST:
							{
								assert(nid!=i);
								//still need a response from them
								waiting_for[nid]++;
								cerr << "MST edge found to "<<i<<endl;
								cerr << " node "<<nid<<" now waiting for "<<waiting_for[nid]<<endl;
								//propegate the search forward
								comms.send(Msg::Type::SRCH, i, nid);
								break;
							}
							case DISCARDED:
							{
								break;
							}

						}
					}

					//NOTE: If we do not send any messages in this block, then
					//we are sure there are no MWOE attached to our subtree. 
					// we need to let someone know! That someone is the parent.
					if (waiting_for[nid]==0)
					{
						//special case, no minimum weight outgoing edge
						cerr << " node "<<nid<<" has no edges to search!"<<endl;
						auto m_best_edge = best_edge[nid];
						cerr <<" >> Best was: "<<m_best_edge<<endl;
						auto parent_link = parent[nid];// WARNING THIS COULD BE US! See SRCH_RET!
						cerr << "Node: " << nid << " sending data back to "<<parent_link<<endl;
						comms.send(Msg::Type::SRCH_RET, parent_link, nid, {m_best_edge.to, m_best_edge.from, m_best_edge.weight});
					}

					break;//end processing SRCH
				}

				case Msg::Type::IN_PART: // are you in my partition? Yes (ACK) or no (NACK)
				{
					auto qpart = m.data[0];
					if (qpart == part_leader[nid])
					{
						//it seems we are in their partition
						cerr << " node "<<nid<<" is part of "<<qpart<<endl;
						comms.send(Msg::Type::ACK_PART, sender, nid, {qpart});
					}
					else
					{
						//it seems we are not
						cerr << " node "<<nid<<" is NOT part of "<<qpart<<endl;
						comms.send(Msg::Type::NACK_PART, sender, nid, {qpart});
					}
					break;
				} // end IN_PART

				case Msg::Type::ACK_PART:
				{

					//any nodes who reply with ACK_PART are already in my partition. Let's ignore them going forward.
					edge_class[nid][sender] = DISCARDED;
					waiting_for[nid]--;
					cerr << " node "<<nid<<" now waiting for "<<waiting_for[nid]<<endl;

					//was that the last response we needed?
					if (waiting_for[nid]==0){

						//if so, let's answer our parent's request for MWOE
						auto m_best_edge = best_edge[nid];
						auto parent_link = parent[nid];
						cerr << "Node: " << nid << " got all respones sending data back to "<<parent_link<<endl;
						comms.send(Msg::Type::SRCH_RET, parent_link, nid, {m_best_edge.to, m_best_edge.from, m_best_edge.weight});
					}
					break;
				} // end ACK_PART

				case Msg::Type::NACK_PART:
				{
					//oh they were NOT in our partition? OK, let's record that helpful information.
					waiting_for[nid]--;
					cerr << " node "<<nid<<" now waiting for "<<waiting_for[nid]<<endl;

					//save the edge from me to the sender
					size_t best_weight=best_edge[nid].weight;
					size_t proposed_weight = connectivity_matrix[nid][sender];

					//but really only if it's better than the best one
					if (best_weight > proposed_weight){
						best_edge[nid].to=sender;
						best_edge[nid].from=nid;
						best_edge[nid].weight=proposed_weight;
					}

					//Are we done looking now? 
					if (waiting_for[nid]==0){
						//yes! Let's propegate up the best found outoing edge
						auto m_best_edge = best_edge[nid];
						auto parent_link = parent[nid];
						cerr << "Node: " << nid << " got all respones sending data back to "<<parent_link<<endl;
						cerr <<" >> Best was: "<<m_best_edge<<endl;
						comms.send(Msg::Type::SRCH_RET, parent_link, nid, {m_best_edge.to, m_best_edge.from, m_best_edge.weight});
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

					cerr << " node " << nid << " waiting for " << waiting_for[nid] << endl;
					//we got an answer to all our SRCH msgs from the downstream nodes who collected ACK_PART and NACK_PART respones
					//NOTE: This could be a resonse from *ourselves*, so be wary of that!
					if (sender != nid)
					{
						//ok, this is from someone else, not just from the previous "stage" of the algorithm
						waiting_for[nid]--;
						cerr << " node " << nid << " now waiting for " << waiting_for[nid] << endl;
						auto m_best_edge = best_edge[nid];
						//let's store their answer, it could be better than ours.

						auto candidate_edge = Edge{m.data[0], m.data[1], m.data[2]};

						cerr<<" node "<< nid << " received "<<candidate_edge<<endl;
						cerr<<" node "<< nid << " comparing to "<<candidate_edge<<endl;
						if (candidate_edge.weight < m_best_edge.weight)
						{
							m_best_edge = candidate_edge;
							best_edge[nid] = candidate_edge;
						}
						cerr<<" node "<< nid << " decision was "<<best_edge[nid]<<endl;
					}

					//now, are we still waiting to communicate to parent?

					//load up the memory for node 'nid'
					auto parent_link = parent[nid];
					auto m_best_edge = best_edge[nid];
					if (waiting_for[nid]==0){

						//special case, am I root?
						if (part_leader[nid]==nid){
							cerr << "Node: "<< nid << " is leader, broadcasting new join_us .. maybe"<<endl;
							if (m_best_edge.weight ==  std::numeric_limits<size_t>::max()){
								cerr << "Node: "<< nid << " is leader, but found no suitable edge, algorithm can terminate! "<<endl;
								//rust, I pine for your oxidization:
								//break main_loop; 	
								converged=true;
							}
							else
							{
								//if we found a new edge let's broadcast the new best edge according to my glorious wishes!
								for (size_t i = 0; i < num_agents; i++)
								{
									//hear ye MST!
									if ( i==nid || edge_class[nid][i] == MST /* && nid!=i The leader should follow join_us commands from itself*/)
									{
										//Let all behold your leader -- who is me -- and tremble
										size_t leader = nid;
										//send forth my commands for node "i" to join the faithful by adding edge 'best_edge' to our MST
										comms.send(Msg::Type::JOIN_US, i, nid, {m_best_edge.to, m_best_edge.from, m_best_edge.weight, leader});
									}
								}
							}
						} else {
							//I'm not leader, but we definitely got all our responses
							cerr << "Node: " << nid << " got all respones sending data back to "<<parent_link<<endl;
							comms.send(Msg::Type::SRCH_RET, parent_link, nid, {m_best_edge.to, m_best_edge.from, m_best_edge.weight});
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
					size_t to = m.data[0];
					size_t from = m.data[1];

					if (from != nid && to !=nid){
						//this command does not apply to us
						//not much for us to do but propegate
						for (size_t i=0;i<num_agents;i++){
							if (edge_class[nid][i]== MST && i!=sender){
								cerr<<" propegating JOIN_US to "<<i<<endl;
								comms.send(m.type, i, nid, m.data);
							}
						}
						break;
					}

					//ok we're one of the important nodes involved in the join
					//first let's mark the edge appropriately
					if (nid == to){
						//edge is incoming, they will ping us when they add and
						//we'll sort out who is parent with NEW_SHERIFF msgs
						edge_class[nid][from]=MST;
					} else if (nid == from){
						//edge is outgoing
						edge_class[nid][to]=MST;
						//tell them about the new boss (my boss)
						size_t my_leader = part_leader[nid];
						size_t wt_for_ref = connectivity_matrix[sender][nid];
						comms.send(Msg::Type::JOIN_US, to, nid, {to /*them*/, from /*me*/, wt_for_ref, my_leader});
						//comms.send(Msg::Type::NEW_SHERIFF, to, nid, {my_leader});
						//NEW_SHERIFF gives them a chance to rebutt with a
						//better choice, see below
					}


					break;
				} // end JOIN_US


				default :{
					cerr<<"=====WARNING UNHANDLED MESSAGE=====";
					cerr<<"Type: "<<m.type<<endl;
					cerr<<"From: "<<sender<<endl;
					cerr<<"To:   "<<nid<<endl;
				} // end default

			}/* switch m.type*/
		}/* while comms.waiting()*/


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
