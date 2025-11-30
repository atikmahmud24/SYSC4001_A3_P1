#!/bin/bash

mkdir output_files 2>/dev/null

for path in ./input_files/input_file*.txt
do
  ./bin/interrupts_EP_RR "$path"
done