#!/bin/sh

for i in {1..10}; do
    ./testlisten > t$i.log 2>&1 &
done

