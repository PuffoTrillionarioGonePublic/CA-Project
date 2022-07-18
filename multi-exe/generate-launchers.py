#!/usr/bin/python3

import os
import os.path
import sys
import subprocess

# diretory to generate launchers in, should be empty
EXEDIR = 'exes'

# test with variable number of threads
# from 1 thread to N included
N = 32

launcher_name = "launch-{n_workers}.sh"
launcher_script = """
#!/bin/bash

CLIENT=multi
EXE=$(dirname $0)/../$CLIENT

# launch multithread application using {n_workers} workers
# first parameter is input file
$EXE "$@" "500" "{n_workers}"

""".lstrip()


def main():
    outdir = os.path.join(os.getcwd(), EXEDIR)
    if os.listdir(outdir):
        print("Directory", outdir, "is not empty!",file=sys.stderr)
        exit(1)

    n_threads = list(range(1,N+1))
    for n_workers in n_threads:
        fname = os.path.join(outdir, launcher_name.format(n_workers=n_workers))
        fcontent = launcher_script.format(n_workers=n_workers)
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

