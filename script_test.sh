#!/bin/bash


if [ $# -lt 1 ]; then
  echo "Usage: $0 <number_of_segments>"
  exit 1
fi

num_segments=$1

if ! [[ "$num_segments" =~ ^[0-9]+$ ]] || [ "$num_segments" -le 0 ]; then
  echo "Error: Please provide a positive integer for the number of segments."
  exit 1
fi

segment_files=()

for ((i=1; i<=num_segments; i++)); do
  segment_files+=("./segments/segment-$i.ts")
done
command="python3 evaluate_test.py ${segment_files[@]}"

echo "Running command: $command"
$command