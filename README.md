# Building

I use cmake.

`apt install cmake`

And building is fairly standard.

I have a few dependencies:

- doctest

which can be fetched locally using `get_deps.sh` on ubuntu. 

Set up cmake:

```
mkdir build
pushd build
cmake ..
popd
```

and build

```
make -C build
```

Optionally, test. To the best of my knoweldge, the easiest eay to test using doctest is to build a test executable and run like this:

```
build/test
```

Code coverage checks and performance testing are not implemented. 

# Installing

There is no `install` target configured at this time. 

# Implementation

## Algorithm

The algorithm implemented is best understood by reading chapter 15.5 of _Distributed Algorithms_ by Lynch. 
In short, each agent begins isolated as leaders of their own partition. Each partition finds an outgoing edge to another partition which is of minimum weight, and the two partitions join up. This continues `log(n)` rounds until all are in the same partition. 
The leader then initiates a search for a new MWOE by all nodes in its partition, and compares returned edges for the minimum weight. The leader broadcasts `JOIN` messages, which are carefully handled by all nodes to ensure that the resulting MST is consistent and correct. The subtleties of this process are elegantly handled, and it's worth reading the chapter to understand what's gong on. 

## Implementation

This library provides a class, `GhsState`, which encapsulates the stateful algorithm execution for `GHS MST` calculation (and leader - election TBD in a new class). It is intended that each agent will instantiate a single `GhsState` class, runtime-static initialized with a fixed number of edges to all possible agents in the environment. It supports dynamically resizing the agent pool, but this is mostly for testing purposes. 

For example, this is the ideal:

```c++
#DEFINE N_AGENTS 5
#DEFINE MY_AGENT_ID 1
#DEFINE STARTING_COMM_LINK EdgeStatus::UNKNOWN
#DEFINE BAD_RSSI 9999

std::vector<Edge> comms_graph = {
  Edge{0,MY_AGENT_ID,STARTING_COMM_LINK,BAD_RSSI},
  Edge{2,MY_AGENT_ID,STARTING_COMM_LINK,BAD_RSSI},
  Edge{3,MY_AGENT_ID,STARTING_COMM_LINK,BAD_RSSI},
  Edge{4,MY_AGENT_ID,STARTING_COMM_LINK,BAD_RSSI},
};

GhsState myState(MY_AGENT_ID,comms_graph);
```

The *use* of the class is simply to process incoming messages. It is assumed that a communications class will accept a number of messages types mapped to processing classes. Here is a simplified example of what I mean:

```c++
class CommunicationsHandler{
  // ...

  /**
  * This is the callback from a select()-like system-call that catches incoming
  * bytes from a radio
  */
  void processMsg(std::list<int8_t> raw_msg){

      //ideally, this object has a mapping of {msg_header->msg_handlers};
      auto handler        = this->get_handler(raw_msg);
      std::deque<Msg> returned_msgs;
      //each msg handler would implement a common super::process(...)
      size_t msgs_to_send = handler.process(raw_msg, &returned_msgs);

      std::deque<Msg> raw_msgs;
      //this class knows how to take message elements and convert to wire-ready
      //bytes, which should not necssarily be // the responsibility of stateful
      //algorithm classes
      size_t msgs_ok      = this->convert_to_raw(returned_msgs,&raw_msgs);

      //error check msgs_ok vs msgs_to_send

      for (const auto *msg: raw_msgs){
          this->send(msg); //<--over the wire, or enqueue for later transmission
      }
  }

  // ...

};

/// and now, this repo's code, implemented for example in in ghs.cpp:

class GhsState: public MsgHandler{ //but we don't extend it yet
   //...
    size_t process(const Msg &msg, std::deque<Msg> *outgoing_buffer);
   //...
};
```

As such, you'll notice all the work is done by `GhsState::process(...)`. *Every other method is used internally and exposed testing only* with the exception of two:

- `start_round()` which is used to trigger a new MST calculation and returns the message to send to trigger all nodes. When this is called is a system implementation detail and is beyond the scope of the class. 
- `reset()` which is used to completely wipe the current state of the MST calculation. Each node becomes isoated and the class is reverted to its startup state after the constructor was called. 
- `set_edge(...)` which is to be used (in an as-yet determined way) to set the communications cost between agents)

However, some informative ones for system implementation might be:

- `get_partition()` which returns a `Partition` type, showing the leader node ID and current algorithm 'level'.

# Style

## File organization

- `hpp` (header) and `cpp` (src files) are in the same folder: `src/`

## Naming conventions

- Types (Structs and Classes) are Capitalized.
- Enums are used when all possible values are known apriori.
- methods are `snake_case`
- class members are `snake_case`
- files are `flying-snake-case`

## Design decisions

### Exceptions, Assertions, Error Codes, and Return Values

- All functions are labeled `noexcept` where the library code will not generate an exception. 
- All functions are labelled `const` where they will not modify internal state.
- Input arguments are validated, rather than trusted. 
- `std::optoonal` is used whenever the caller requests data that may not exist but the caller would not necessarily have known that. This provides a concise way of both retrieving and testing for the existence of a member. 
- Each function or constructor specifies (in comments) the assumptions and pre/post conditions. A violation of preconditions or assumptoions on input will generate an Exception.  For example, if the class `A` provides a `size()` method and is list-like, then requesting more than `size()` elements will generate an exception -- it's not possible and caller should have known better.
- An assertion will trigger to indicate a programmer error that results in an unworkable state, rather than trying to recover ot a partially useable state. 
- More specifically, Assertions are used to check the internal (not user modifyable) state of the library. For example, if the internal state does not support the operation, but the caller provided good data and the library was in a good state prior, then there is nothing to do but crash, as the current object is obviously corrupted. 
- Return values are used to indicate a side effect has been triggered. For example, returning the number of internal structures added. This is helpful for caller code testing their assumptions. 

### Signatures, References, and Pointers

- `int set_X()` and `X get_X()` are preferred over `void set(X)` and `X X()`. Often, the int returns some useful information about what was modified internally, or `void` if no such information is possible. Compare `set_edge` vs `set_parent_edge`. 
- Pointers are used to indicate a variable that *is expected to be modified*. References are preferred over values when passing arguments.
- `std::shared_ptr` is **not currently** used over raw points, but can easiliy be changed

### Data structures

- Wherever possible, hash-based data structures are avoided until I can get approval to use them. There are places in the code where a `std::unordered_set` would have made life much easier, but given the small numbers of agents for our use case, I often use lists instead. 
- When `N` is known, all refs to `std::vector<T>` should be replaced with `std::array<T,N>`, and all `std_unordered_set<T>` can become `std::array<T,N>` as well. 



