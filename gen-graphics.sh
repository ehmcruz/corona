#!/bin/bash

# if [ "$#" -ne 2 ]; then
# 	echo "Usage: ./run-sim.sh <test_name> <number_of_runs>"
# 	exit
# fi

#sceneries="pvai-do-nothing pvai-2-waves"
#sceneries="palmas-do-nothing palmas-2-waves"
sceneries="palmas-do-nothing"

for test in $sceneries
do
	./log/calc-mean-stddev.r log/results/results-$test log/results/results-$test
	
	./log/plot-with-interval.r log/results/results-$test-mean.csv log/results/results-$test-se.csv log/results/results-$test
done
