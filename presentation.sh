#!/bin/bash

readonly FILES=9

for i in `seq 1 $FILES`;
do
    cp file_small.cpp small_$i.cpp
done

for i in `seq 1 $FILES`;
do
    cp file_big.cpp big_$i.cpp
done

set -x
./fs -ls

./fs -create 10000

./fs -ls
./fs -lm

for i in `seq 1 $FILES`;
do
    ./fs -u big_$i.cpp
    ./fs -u small_$i.cpp
    ./fs -ls
    ./fs -lm
done

./fs -rm big_3.cpp
./fs -ls
./fs -lm
./fs -rm small_1.cpp
./fs -rm small_6.cpp
./fs -ls
./fs -lm

./fs -u small_1.cpp




./fs -destroy
set +x

find ./ -name 'small_*' -exec rm {} \;
find ./ -name 'big_*' -exec rm {} \;
