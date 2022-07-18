#!/bin/bash


for I in {0..9};
do
sudo time nvprof ./kernel-no-opt ../inputs/dataset-256-10000.csv 3>&2 2>&1 1>&3 >/dev/null | tee nvidia-profile-no-opt-256-10K-rep${I}.txt
done
