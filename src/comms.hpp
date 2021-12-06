#include <deque>
#include <iostream>
#include <fstream>
#include <vector>
#include "msg.hpp"



//system stuff / simulate comms
class Comms
{
    public:
    Comms(){
        verbose=true;
        fname="msgs.msgs";
        ofile = std::ofstream(fname,std::fstream::out | std::fstream::app);
        n_msgs_sent=0;
        n_amt_sent=0;
    }

    void set_verbose(bool v){
        verbose=v;
    }

    void set_logfile(std::string fname){
        ofile = std::ofstream(fname,std::fstream::out | std::fstream::app);
    }

	void send(Msg::Type type, size_t to, size_t from, std::vector<size_t> data = {})
	{

        Msg m {type, to, from, data};
        if (verbose){
            ofile<<m<<std::endl;
            std::cerr<<"(++"<<msg_q.size()<<")"<<"Sending: "<<m<<std::endl;
        }
		msg_q.emplace_back(m);
		n_msgs_sent++;
		n_amt_sent += data.size() + 2;
	}

	size_t waiting() { return msg_q.size(); }
  size_t bytes_sent(){return this->n_amt_sent; }
  size_t msgs_sent(){return this->n_msgs_sent; }

	Msg get()
	{
		Msg r = msg_q.front();
        if (verbose){
            std::cerr<<"(--"<<msg_q.size()<<")"<<"Processing: "<<r<<std::endl;
        }
		msg_q.pop_front();
		return r;
	}

private:
	bool verbose;
	std::deque<Msg> msg_q;
	size_t n_msgs_sent;
	std::string fname;
	std::ofstream ofile;
	size_t n_amt_sent;

};
