#!/bin/sh
if [ -d deps/ ]
then
	rm deps -rf
fi
mkdir deps
git clone https://github.com/onqtam/doctest deps/doctest
