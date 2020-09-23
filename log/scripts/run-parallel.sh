#!/bin/bash

cmd="$1"

#sceneries="parana-do-nothing"
#sceneries="parana-current"
#sceneries="14reg-current"
sceneries="sp"

run_ini="1"
run_end="1"

run_list=`seq $run_ini 1 $run_end`

#run_list="3"

vc="420"
vw="2"
vb="30"
vs="0"
vp="0.5"
vo="210"
vv="5000"
vz="none"

function exec_exp () {
	vc="$1"
	vw="$2"
	vb="$3"
	vs="$4"
	vp="$5"
	vo="$6"
	vv="$7"
	vz="$8"

	echo "vc=$vc   vw=$vw   vb=$vb   vs=$vs   vp=$vp   vo=$vo   vv=$vv   vz=$vz"

	for test in $sceneries
	do
		mkdir -p log/results/

		base="$test-c$vc-w$vw-b$vb-s$vs-p$vp-o$vo-v$vv-z$vz"

		#rm -rf log/results/all-results-$base
		mkdir log/results/all-results-$base

		if [ "$cmd" == "blaise" ]; then
			echo "" >> $fout
			echo "mkdir log/results/all-results-$base" >> $fout
		fi

		for i in $run_list
		do
			fresults="log/results/all-results-$base/results-$base-$i"
			cmdline="./scenery/$test.exec --fresults $fresults -c $vc -w $vw -b $vb -s $vs -p $vp -o $vo -v $vv -z $vz"

			if [ "$cmd" == "blaise" ]; then
				echo "" >> $fout
				echo "$cmdline &" >> $fout
			elif [ "$cmd" == "run" ]; then
				if [ ! -f "$fresults-global.csv" ]; then
					sbatch run-shared.sbatch "$cmdline"
					#echo "$cmdline"
				fi
			else
				echo "error cmd $cmd not found"
				exit
			fi
		done

		if [ "$cmd" == "blaise" ]; then
			echo "wait" >> $fout
		fi
	done
}

fout="blaise.sbatch"

if [ "$cmd" == "blaise" ]; then
	echo "#!/bin/bash" > $fout
	echo "#SBATCH --ntasks=1" >> $fout
	echo "#SBATCH --partition=blaise" >> $fout
	echo "#SBATCH --time=02:00:00" >> $fout
	echo "#SBATCH --output=%x_%j.out" >> $fout
	echo "#SBATCH --error=%x_%j.err" >> $fout

	chmod +x $fout
fi
#exit

################################################################

# exp baseline
# no schools

vc="420"
vb="30"
vs="0"
vp="0.5"
vo="210"
vv="5000"
vw="2"
vz="none"

exec_exp $vc $vw $vb $vs $vp $vo $vv $vz

################################################################

# exp figure 1
# all students go

vc="420"
vb="30"
vs="100"
vp="0.5"
vo="210"
vv="5000"
vz="none"

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz
done

################################################################

# exp figure 2
# sp plan

vc="420"
vb="30"
vs="planned"
vp="0.5"
vo="210"
vv="5000"
vz="none"

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz
done

#exit

################################################################

# exp figure 3
# sp plan varying social distance

vc="420"
vb="30"
vs="planned"
vo="210"
vv="5000"
vz="none"

#for vp in 0.0 0.1 0.2 0.3 0.4 0.6 0.7 0.8 0.9 1.0
for vp in 0.0 0.2 0.4 0.6 0.8 1.0
do
	for vw in 2 4
	do
		exec_exp $vc $vw $vb $vs $vp $vo $vv $vz
	done
done

################################################################

# exp 4 - vaccine

# no vaccine
# no schools

vc="600"
vb="30"
vs="0"
vp="0.5"
vo="330"         # open in feb/2021 = 11*30
vv="300"         # vaccine in january
vw="2"
vz="none"

exec_exp $vc $vw $vb $vs $vp $vo $vv $vz

# no vaccine
# all students go

vs="100"

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz
done

# vaccine - random
# all students go

vz="random"
vs="100"

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz
done

# vaccine - priorities
# all students go

vz="priorities"
vs="100"

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz
done

