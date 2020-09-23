#!/bin/bash
#SBATCH --ntasks=1
#SBATCH --partition=sirius
#SBATCH --exclusive
#SBATCH --exclude=beagle,orion1,orion2,draco3
#SBATCH --time=02:00:00
#SBATCH --output=%x_%j.out
#SBATCH --error=%x_%j.err

#no SBATCH --partition=sirius,shared
#no SBATCH --nodelist=draco7

cmdline="$1"

$cmdline
