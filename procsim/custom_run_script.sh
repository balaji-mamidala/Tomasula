#!/bin/bash

OP='test.csv'	
run=1
F=4
echo "R|K0|k1|k2|F|Fil|IPC" >> $OP
while [ $F -le 8 ]; do
	J=1
	while [ $J -le 2 ]; do
		K=1
		while [ $K -le 2 ]; do
			L=1
			while [ $L -le 2 ]; do
				R=1
				while [ $R -le $((J+K+L)) ]; do
					for i in `ls ./traces/`; do
						./procsim -j $J -k $K -l $L -f $F -r $R -i ./traces/$i >> $OP
						echo Run=$run :: completed F=$F K0=$J K1=$K K2=$L R=$R File=$i
						run=$((run + 1))					
					done;
					R=$((R + 1))  
				done;
				L=$((L + 1))  
			done;
			K=$((K + 1))  
		done;
		J=$((J + 1))  	  
	done;	
	F=$((F + 4))  
done;
