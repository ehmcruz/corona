#!/bin/bash

# if [ "$#" -ne 2 ]; then
# 	echo "Usage: ./run-sim.sh <test_name> <number_of_runs>"
# 	exit
# fi

#sceneries="pvai-2-waves"
#sceneries="14reg-do-nothing"
sceneries="14reg-current"
#sceneries="pvai-do-nothing pvai-2-waves"
#sceneries="pvai-do-nothing"
#sceneries="palmas-do-nothing"
nruns="5"

subpop="global Paranavai school"

for test in $sceneries
do
	mkdir -p log/results/
	
	rm -rf log/results/all-results-$test
	mkdir log/results/all-results-$test

	for i in `seq 1 $nruns`
	do
		echo "running $test run $i"
		
		./scenery/$test.exec --fresults log/results/all-results-$test/results-$test-$i
	done
done
