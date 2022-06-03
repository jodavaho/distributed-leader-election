# What is this

**This documentation best read in doxygen**. 

If you build on your system, that will be in [docs/html/index.html](docs/html/index.html).

NTR 52251, titled "Library for Leader Election and Spanning Tree Distributed Construction", submitted by Joshua Vander Hook 12/14/2021

This rep provides a few things:

- A C++ GHS implementation library `libghs.so`
- A C++ statically-sized messager queue that is based on a circular buffer `libghs_seque.so`
- An optional set of extensions that help to build algorithms around it `libghs_ext.so` (configure with `-DBUILD_EXT=On`)
- An optional set of tools to help with studies of performance on various graphs, and to output meaningful visuals using the `dot` linux utility `to-dot`, `random-graph`, `ghs-score` (configure with `-DBUILD_TOOLS=On`)
- An optional doctest executable, that will unit-test the currently built library `ghs-doctest` (configure with `-DBUILD_DOCTEST=On`)
- An optional demo executable, that is suitable for integration-type testing on multi-machine, multi-process environments `ghs-demo` (configure with `-DBUILD_DEMO=On`)
- An experimental integration of `miniz` to do message compressing in `ghs-demo` (configure with `-DENABLE_COMPRESSION=On`)

# Dependencies

The union of all dependencies can be obtained by running `get_deps.sh` on an ubuntu-like system.

- `libghs.so` and `libghs_seque.so`: `C++11`
- `libghs_ext.so`: as above, and `std::`
- The tools: all of previous
- `ghs-doctest`: all of previous, and `libdoctest-dev` or a similar doctest installation
- `ghs-demo`: all of previous, and `libnng-dev`, `libinih-dev` and `miniz` checked out in `ext/miniz` if compression is enabled

There's also ROS support, which brings with it a lot of dependencies not handled by this repo or `get_deps.sh`

# Configuring

I use cmake.  `apt install cmake`

To configure,

```
mkdir build
pushd build
cmake .. -D<option>=On -D<option 2>=On 
popd  
```

See `CMakeLists.txt` for details, but the set of `<options>` are:

- `BUILD_DOCS` (default=On): build doxygen docs
- `BUILD_EXT` (default=Off): build `libghs_ext.so`
- `BUILD_DOCTEST` (default=Off): build `ghs-doctest`. Implies `BUILD_EXT`
- `BUILD_DEMO` (default=Off): build `ghs-demo`. Implies `BUILD_EXT`
- `ENABLE_ROS` (default=Off): Add some CMake sugar to play well with catkin and ROS
- `ENABLE_COMPRESSION` (default=Off): Try out experimental (i.e., not really working) libz compression 
- `BUILD_TOOLS` (default=Off): build the utilities `ghs-score`, `to-dot`, and `random-graph` for testing and visualization.

Code coverage checks and performance testing are not implemented. 

# Trying it out using ghs-demo

You can try it out on various machines. You'll have to set up a config that describes the network, then run `ghs-demo` on each machine. you can run them all locally, just set the agent endpoints to something like `tcp://localhost:<a port per agent>` or `ipc:///tmp/agent0`, `ip:///tmp/agent1` etc

This should work fine for a the `le_config.ini` file:

```ini
[agents]
; valid endpoints are sockets, so
; tcp://hostname:port 
; ipc://<file>
; are both valid
; Agents must have consecutive ids starting at 0
0=tcp://localhost:5000
1=tcp://localhost:5001
2=tcp://localhost:5002
3=tcp://localhost:5003

; Or locally:
;0=ipc:///tmp/agent0
;1=ipc:///tmp/agent1
;2=ipc:///tmp/agent2
;3=ipc:///tmp/agent3

; Or for real fun, on diff machines
;0=tcp://192.168.1.100:5000
;0=tcp://192.168.1.101:5000
;0=tcp://192.168.1.102:5000
;0=tcp://192.168.1.103:5000

[runtime]
retry_connections=false
```

Then, in four terminals, execute:

`<le_config.ini ghs-demo --start -i <ID> -w <S>`

(usually ghs-demo is in `build/src/demo/`)

where:

- `<ID>` is the id of the current node (so we know where to listen)
- `<S>` is an optional wait-time in seconds. The node will wait before starting the algorithm this many seconds, to give you a chance to start all nodes. All nodes should be started before any wait timer expires!!

If it works, you'll see `Converged!!` in all windows, and after a few seconds it should shut down.  You can also look at the step-by-step edges used by each node in the output stream, if you have the stomach to look through it.

# Installing

There is no `install` target configured at this time. 

# Implementation

You don't need to read any of the following, but you may be interested.

## Algorithm

The algorithm implemented is best understood by reading chapter 15.5 of _Distributed Algorithms_ by Lynch. 
In short, each agent begins isolated as leaders of their own partition. Each partition finds an outgoing edge to another partition which is of minimum weight, and the two partitions join up. This continues `log(n)` rounds until all are in the same partition. 

Each round, the leader initiates a search for a new MWOE by all nodes in its partition, and compares returned edges for the minimum weight. The leader broadcasts `JOIN` messages, which are carefully handled by all nodes to ensure that the resulting MST is consistent and correct. The subtleties of this process are elegantly handled, and it's worth reading the chapter to understand what's gong on. (though I read it many times and may not fully grok all the corner cases yet)

## Implementation

A working implementation is provided by `ghs-demo`, which can be built by configuring with `-DBUILD_DEMO=On`. 

See the documentation of `ghs-demo.h`, in particular `demo::GhsDemoExec` for full implementation details. 

# Style

## File organization

- `h` (header) and `cpp` (src files) are in the same folder: `src/`
- Implementation headers `hpp` are, too
- This is not always well observed under `src/ghs-demo`

## Naming conventions

- Types (Structs and Classes) are Capitalized.
- Enums are used when all possible values are known apriori.
- methods are `snake_case`
- class members are `snake_case`
- files are `flying-snake-case`


# Copyright

Copyright (c) 2021 California Institute of Technology (“Caltech”).  U.S.
Government sponsorship acknowledged.

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

*  Neither the name of Caltech nor its operating division, the Jet Propulsion
   Laboratory, nor the names of its contributors may be used to endorse or
   promote products derived from this software without specific prior written
   permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# License

- Apache-2.0

See [LICENSE.md](LICENSE.md)

