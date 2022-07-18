#!/usr/bin/python3

import math
import os
import os.path
import sys
import subprocess

# diretory to generate launchers in, should be empty
EXEDIR = 'exes'

# test with variable number of threads
# from 1 thread to 2**N included
N = 16

# no more than MAX_THEAD_PER_BLOCK thread per block
MAX_THEAD_PER_BLOCK = 1024

# list containing all the combinations of (blocks,trhead per block used)
pairs_blk_tpb = []

def gen_pairs_blk_tpb():
    ans = []
    for pow in range(N+1):
        tot_thread = 2**pow
        for t_pow in range(min(pow+1, int(math.log(MAX_THEAD_PER_BLOCK, 2)+1))):
            tpb = 2**t_pow
            blk = tot_thread//tpb
            ans.append((blk, tpb))
    return ans

pairs_blk_tpb = gen_pairs_blk_tpb()
#print(len(pairs_blk_tpb), pairs_blk_tpb)

# generate one pair for each 
def gen_few_pairs_blk_tpb(thread_N, max_tpb):
    ans = []
    # initially one block with one thread
    blk = 1; tpb = 1;
    for pow in range(thread_N+1):
        ans.append((blk, tpb))
        if tpb < max_tpb:
            tpb *= 2
        else:
            blk *=2
    return ans

launcher_name = "launch-{blk}-{tpb}.sh"
launcher_script = """
#!/bin/bash

CLIENT=kernel
EXE=$(dirname $0)/../$CLIENT

# launch cuda application using (n.blocks: {blk}, n.thread per block: {tpb})
# first parameter is input file
$EXE "$@" "{blk}" "{tpb}"

""".lstrip()


def main():
    outdir = os.path.join(os.getcwd(), EXEDIR)
    if os.listdir(outdir):
        print("Directory", outdir, "is not empty!",file=sys.stderr)
        exit(1)

    pairs = gen_few_pairs_blk_tpb(N, MAX_THEAD_PER_BLOCK)
    for blk,tpb in pairs:
        fname = os.path.join(outdir, launcher_name.format(blk=blk, tpb=tpb))
        fcontent = launcher_script.format(blk=blk, tpb=tpb)
        with open(fname, mode='w') as f:
            print('Created:', fname)
            f.write(fcontent)
            #print(fcontent)
            print()
        res = subprocess.run(['chmod', '+x', fname])
        if res.returncode != 0:
            print("Failed: chmod +x", fname, file=sys.stderr)
            print("Res", res, file=sys.stderr)
            exit(1)

if __name__ == "__main__":
    main()

