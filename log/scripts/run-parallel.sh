#!/bin/bash

cmd="$1"

#sceneries="parana-do-nothing"
#sceneries="parana-current"
#sceneries="14reg-current"
sceneries="sp"

run_ini="1"
run_end="10"

#run_list="3"

vc="730"
vw="2"
vb="28"
vs="0"
vp="0.5"
vo="224"
vv="5000"
vz="none"
vicu="100000000"
va="homogeneous"

if [ "$cmd" == "cmd" ]; then
	run_ini="1"
	run_end="1"
fi

run_list=`seq $run_ini 1 $run_end`

function exec_exp () {
	vc="$1"
	vw="$2"
	vb="$3"
	vs="$4"
	vp="$5"
	vo="$6"
	vv="$7"
	vz="$8"
	vicu="$9"
	va="${10}"

	echo "vc=$vc   vw=$vw   vb=$vb   vs=$vs   vp=$vp   vo=$vo   vv=$vv   vz=$vz   vicu=$vicu   va=$va"

	for test in $sceneries
	do
		mkdir -p log/results/

		base="$test-c$vc-w$vw-b$vb-s$vs-p$vp-o$vo-v$vv-z$vz-icu$vicu-a$va"

		#rm -rf log/results/all-results-$base
		mkdir log/results/all-results-$base

		if [ "$cmd" == "blaise" ]; then
			echo "" >> $fout
			echo "mkdir log/results/all-results-$base" >> $fout
		fi

		for i in $run_list
		do
			fresults="log/results/all-results-$base/results-$base-$i"
			cmdline="./scenery/$test.exec --fresults $fresults -c $vc -w $vw -b $vb -s $vs -p $vp -o $vo -v $vv -z $vz --spicu $vicu -a $va"

			if [ "$cmd" == "blaise" ]; then
				echo "" >> $fout
				echo "$cmdline &" >> $fout
			elif [ "$cmd" == "run" ]; then
				if [ ! -f "$fresults-global.csv" ]; then
					#echo "$cmdline"
					sbatch run-shared.sbatch "$cmdline"
				fi
			elif [ "$cmd" == "cmd" ]; then
				echo "$cmdline"
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

vc="730"
vb="28"
vs="0"
vp="0.5"
vo="224"
vv="5000"
vw="2"
vz="none"

for va in homogeneous children asymp
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
done

################################################################

# exp figure 1
# all students go

vc="730"
vb="28"
vs="100"
vp="0.5"
vo="224"
vv="5000"
vz="none"

for va in homogeneous children asymp
do
	for vw in 2 4
	do
		exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
	done
done

################################################################

# exp figure 2
# sp plan

vc="730"
vb="140"
vs="planned"
vp="0.5"
vo="224"
vv="5000"
vz="none"

for va in homogeneous children asymp
do
	for vw in 2 4
	do
		exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
	done
done

#exit

################################################################

# exp figure 3
# sp plan varying social distance

vc="730"
vb="140"
vs="planned"
vo="224"
vv="5000"
vz="none"
va="homogeneous"

#for vp in 0.0 0.1 0.2 0.3 0.4 0.6 0.7 0.8 0.9 1.0
for vp in 0.0 0.2 0.4 0.6 0.8 1.0
do
	for vw in 2 4
	do
		exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
	done
done

################################################################

# exp 4 - vaccine in january

# no vaccine
# no schools

vc="730"
vb="28"
vs="0"
vp="0.5"
vo="341"         # open in feb/2021
vv="310"         # vaccine in january/2021
vw="2"
vz="none"
va="homogeneous"

exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va

# vaccine
# no schools

vz="priorities"

exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va

# no vaccine
# all students go

vz="none"
vs="100"

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
done

# vaccine - random
# all students go

# vz="random"
# vs="100"

# for vw in 2 4
# do
# 	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
# done

# vaccine - priorities
# all students go

vz="priorities"
vs="100"

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
done

# sp plan with vaccines

#vc="730"
#vb="28"
#vs="planned"
#vp="0.5"
#vo="224"
#vv="310"
#vz="priorities"

#for vw in 2 4
#do
#	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
#done

################################################################

# exp 5 - vaccine in march

# no vaccine
# no schools

vc="730"
vb="28"
vs="0"
vp="0.5"
vo="400"         # open in aprl/2021
vv="369"         # vaccine in march/2021
vw="2"
vz="none"
va="homogeneous"

exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va

# vaccine
# no schools

vz="priorities"

exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va

# no vaccine
# all students go

vz="none"
vs="100"

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
done

# vaccine - random
# all students go

# vz="random"
# vs="100"

# for vw in 2 4
# do
# 	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
# done

# vaccine - priorities
# all students go

vz="priorities"
vs="100"

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
done

# sp plan with vaccines

#vc="730"
#vb="28"
#vs="planned"
#vp="0.5"
#vo="224"
#vv="310"
#vz="priorities"

#for vw in 2 4
#do
#	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
#done

################################################################

# exp figure 6
# sp plan

vc="730"
vb="145"
vs="planned"
vp="0.5"
vo="224"
vv="369"
vz="priorities"
va="homogeneous"

# (369 + 14 + 14) - (224+28) = 145

for vw in 2 4
do
	exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va
done

vs="0"

exec_exp $vc $vw $vb $vs $vp $vo $vv $vz $vicu $va

#exit

