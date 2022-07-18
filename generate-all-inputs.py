#!/usr/bin/python3

import sys
import argparse
import os
import os.path
# https://docs.python.org/3/library/subprocess.html#using-the-subprocess-module
import subprocess
import itertools

# 10'000 100'000 1'000'000
ROW_N = [10**i for i in range(4,7)]
COL_N = [2**i  for i in range(1,9)]

# script generating *.csv files
generator = "csv.sh"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output_dir', type=str, help='Path of output directory')
    args = parser.parse_args()
    print(f"Running with args: {args}")
    if os.path.isdir(args.output_dir):
        print(f"Error: path \"{args.output_dir}\" already exists", file=sys.stderr)
        exit(1)

    # get absolute path to the script generating the *.csv
    csvscript = os.path.abspath(generator)
    # check file existence
    if not os.path.exists(csvscript):
        print(f"Error: file \"{generator}\" not found in path {csvscript}", file=sys.stderr)
        exit(1)

    ## create and move to new directory
    os.mkdir(args.output_dir)
    os.chdir(args.output_dir)

    for c,r in itertools.product(COL_N, ROW_N):
        c = str(c)
        r = str(r)
        filename = f'dataset-{c}-{r}.csv'
        with open(filename, mode='w') as fout:
            ofd = fout.fileno()
            args = [csvscript, c, r]
            subprocess.call(args, stdout=ofd)

if __name__ == "__main__":
    main()
