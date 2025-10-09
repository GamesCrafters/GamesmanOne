#!/bin/bash

for i in $(seq 0 143); do
    /usr/bin/time bin/gamesman solve mills "$i"
    /usr/bin/time bin/gamesman analyze mills "$i"
done
