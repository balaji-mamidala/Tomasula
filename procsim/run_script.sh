#!/bin/bash

for i in `ls ./traces/`; do
	./procsim -r 2 -j 3 -k 2 -l 1 -f 4 -i ./traces/$i > ./my_output/$i.log
	echo $i executed 
done;
	echo program terminating
	




