#!/bin/bash


for I in {0..9};
do
sudo time nvprof ./kernel ../inputs/dataset-256-100000.csv 3>&2 2>&1 1>&3 >/dev/null | tee nvidia-profile-256-100K-rep${I}.txt
done
