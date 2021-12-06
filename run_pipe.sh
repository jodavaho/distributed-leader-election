#!/bin/bash
n=9

build/random-graph $n | build/to-dot | sfdp -Tpng -o ~/Desktop/$n-graph.png
build/random-graph $n | build/ghs-score | build/to-dot | sfdp -Tpng -o ~/Desktop/$n-ghs.png
build/random-graph $n | build/ghs-score | build/stleader | build/to-dot | dot -Tpng -o ~/Desktop/$n-ghs-st.png


build/random-graph $n | build/ghs-score | build/middle-leader| build/to-dot | dot -Tpng -o ~/Desktop/$n-ghs-mid.png
build/random-graph $n > /tmp/graph &&  time < /tmp/graph build/ghs-score
