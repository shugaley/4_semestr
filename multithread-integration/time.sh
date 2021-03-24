#!/bin/bash

exec 2> build/time.txt

for ((i = 1; i <= $1; i++))
do
#echo '==== Thread number $i' > build/time.txt
time ./build/calc.out $i
done
