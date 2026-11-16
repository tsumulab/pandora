#!/bin/bash

Benches=("genome" "intruder" "kmeans_high" "kmeans_low" "labyrinth" "ssca2" "vacation_high" "vacation_low" "yada")

Params=("-g16384 -s64 -n16777216") # genome
Params+=("-a10 -l128 -n262144 -s1") # intrueder
Params+=("-m15 -n15 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt") # kmeans_high
Params+=("-m40 -n40 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt") # kmeans_low
Params+=("-i labyrinth/inputs/random-x512-y512-z7-n512.txt") # labyrinth
Params+=("-s20 -i1.0 -u1.0 -l3 -p3") # ssca2
Params+=("-n4 -q60 -u90 -r1048576 -t4194304") # vacation_high
Params+=("-n2 -q90 -u98 -r1048576 -t4194304") # vacation_low
Params+=("-a15 -i yada/inputs/ttimeu1000000.2") # yada

for i in {0..8}; do
  if [[ $2 = ${Benches[i]} ]]; then
    if [[ $i == 2 ]] || [[ $i == 3 ]]; then
      build/$1/kmeans ${Params[i]} -p$3
    elif [[ $i == 6 ]] || [[ $i == 7 ]]; then
      build/$1/vacation ${Params[i]} -c$3
    else
      build/$1/${Benches[i]} ${Params[i]} -t$3
    fi
    break
  fi
done
