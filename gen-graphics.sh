#!/bin/bash

# if [ "$#" -ne 2 ]; then
# 	echo "Usage: ./run-sim.sh <test_name> <number_of_runs>"
# 	exit
# fi

gsize="8"

#sceneries="pvai-do-nothing pvai-2-waves"
sceneries="pvai-do-nothing"
#sceneries="palmas-do-nothing palmas-2-waves"
#sceneries="palmas-do-nothing"

for test in $sceneries
do
	rm -rf log/results/processed-$test
	mkdir log/results/processed-$test

	for f in `ls log/results/results-$test`
	do
		fin="log/results/results-$test/$f"
		fout="log/results/processed-$test/$f"
		echo "$fin -> $fout"
		./log/calculate-stuff.r $fin $fout $gsize
	done

#	exit

	./log/calc-mean-stddev.r log/results/processed-$test log/results/results-$test
	
	./log/plot-with-interval.r log/results/results-$test-mean.csv log/results/results-$test-se.csv log/results/results-$test
done
