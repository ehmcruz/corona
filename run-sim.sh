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
nruns="10"

subpop="global Paranavai school"

for test in $sceneries
do
	mkdir -p log/results/
	
	for s in $subpop
	do
		rm -rf log/results/results-$test-$s
		mkdir log/results/results-$test-$s
	done

	for i in `seq 1 $nruns`
	do
		echo "running $test run $i"
		
		./scenery/$test.exec results-$test-$i
		
		for s in $subpop
		do
			mv results-$test-$i-$s.csv log/results/results-$test-$s
		done
	done
done
