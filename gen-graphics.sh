#!/bin/bash

# if [ "$#" -ne 2 ]; then
# 	echo "Usage: ./run-sim.sh <test_name> <number_of_runs>"
# 	exit
# fi

gsize="9"

#sceneries="pvai-do-nothing pvai-2-waves"
#sceneries="pvai-2-waves"
#sceneries="pvai-do-nothing"
#sceneries="14reg-do-nothing"
#sceneries="parana-do-nothing-simple"
#sceneries="parana-current"
sceneries="14reg-current"
#sceneries="palmas-do-nothing palmas-2-waves"
#sceneries="palmas-do-nothing"

subpop="global Paranavai school"

for test in $sceneries
do
	for s in $subpop
	do
		rm -rf log/results/results-$test-$s
		mkdir log/results/results-$test-$s

		cp log/results/all-results-$test/results-$test-*-$s.csv log/results/results-$test-$s
	done

	for s in $subpop
	do
		rm -rf log/results/processed-$test-$s
		mkdir log/results/processed-$test-$s

		for f in `ls log/results/results-$test-$s`
		do
			fin="log/results/results-$test-$s/$f"
			fout="log/results/processed-$test-$s/$f"
			echo "$fin -> $fout"
			./log/calculate-stuff.r $fin $fout $gsize
		done

	#	exit

		rm -rf log/results/graphics-$test-$s
		mkdir log/results/graphics-$test-$s
		
		./log/calc-mean-stddev.r log/results/processed-$test-$s log/results/graphics-$test-$s/results-$test-$s

		./log/plot-with-interval.r log/results/graphics-$test-$s/results-$test-$s-mean.csv log/results/graphics-$test-$s/results-$test-$s-se.csv log/results/graphics-$test-$s/results-$test-$s
	done

	# clean
	
	for s in $subpop
	do
		rm -rf log/results/results-$test-$s
		rm -rf log/results/processed-$test-$s
	done
done
