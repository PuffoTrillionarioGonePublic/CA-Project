#!/usr/bin/python3

import sys
import argparse
import os
import os.path
# https://docs.python.org/3/library/subprocess.html#using-the-subprocess-module
import subprocess
import resource
import itertools
import time
import datetime


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('exe_dir', type=str, help='Path of directory containing all executables')
	parser.add_argument('in_dir', type=str, help='Path of directory containing all input datasets')
	parser.add_argument('out_file', type=str, help='Path to the file to write the output')
	parser.add_argument('repetitions', type=int, default=1, help='Path of directory containing all executables')
	args = parser.parse_args()
	print(f"Running with args: {args}")

	if not os.path.isdir(args.exe_dir):
		print(f"Error: file \"{args.exe_dir}\" not found", file=sys.stderr)
		exit(1)
	exes = [os.path.join(args.exe_dir, fn) for fn in os.listdir(args.exe_dir) if os.path.isfile(os.path.join(args.exe_dir, fn)) and os.access(os.path.join(args.exe_dir, fn),  os.X_OK)]

	# check file existence
	if not os.path.isdir(args.in_dir):
		print(f"Error: file \"{args.in_dir}\" not found", file=sys.stderr)
		exit(1)
	inputs = [os.path.join(args.in_dir, fn) for fn in os.listdir(args.in_dir) if os.path.isfile(os.path.join(args.in_dir, fn))]

	if os.path.exists(args.out_file):
		print(f"Error: file \"{args.out_file}\" already exists", file=sys.stderr)
		exit(1)

	print("Inputs:", len(inputs), inputs)
	print("Exes:  ", len(exes),   exes)

	with open(args.out_file, "w") as f:
		# write csv header
		f.write("timestamp,exe,input,repetition,real_time,user_time,sys_time\n")
		# debugging
		print(" timestamp,exe,input,repetition,real_time,user_time,sys_time\n")

		# test all (exe, input) pairs multiple times
		for rep in range(args.repetitions):
			for exe, input in itertools.product(exes, inputs):
				print(f"running: {exe=:<25} {input=:<25} {rep=:<3}")

				rusage_start = resource.getrusage(resource.RUSAGE_CHILDREN)
				time_begin=datetime.datetime.now()
				subprocess.call([exe, input], stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
				time_end=datetime.datetime.now()
				real_time=time_end-time_begin
				real_time=real_time.seconds+real_time.microseconds/10**6
				rusage_end = resource.getrusage(resource.RUSAGE_CHILDREN)
				user_time = rusage_end.ru_utime - rusage_start.ru_utime
				sys_time = rusage_end.ru_stime - rusage_start.ru_stime
				# write csv line
				f.write(f"{time.time():.10f},{exe},{input},{rep},{real_time:.6f},{user_time:.6f},{sys_time:.6f}\n")
				f.flush()
				# usefull for debugging
				print(f" {time.time():.10f},{exe},{input},{rep},{real_time:.6f},{user_time:.6f},{sys_time:.6f}\n")

if __name__ == "__main__":
	main()	
