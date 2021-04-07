#!/bin/bash

exec 2> build/time.txt

# for ((i = 1; i <= $1; i++))
# do
#     time ./build/calc.out $i
# done

# time ./build/calc.out 32
# time ./build/calc.out 64
# time ./build/calc.out 1200

for i in 1 2 3 4 5 6 7 8 9 10 11 12 16 32 64 1200
do
    time ./build/calc.out $i
done
