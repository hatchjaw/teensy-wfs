#!/bin/bash

if ! command -v tycmd &>/dev/null; then
  echo -e "Error: requires tytools, which were not found https://github.com/Koromix/tytools\n" >&2
  exit 1
fi

environment=$1
if [ -z "$environment" ]; then
  echo "Expected to receive a platformIO environment. Defaulting to 'wfs'."
  environment="wfs"
fi

# Build
cd "$(dirname "$(realpath "$0")")"/.. || exit 1
# Run
pio run -e "$environment"
# GTFO if pio's exited unhappily
if [ $? -eq 1 ]; then
    exit 1
fi

cd .pio/build/$environment || exit 1

echo -e "\nLooking for Teensies to upload to."

# Query all connected Teensies
teensies=($(tycmd list | grep -Eo "[0-9]+-Teensy"))

# Upload
if ((${#teensies[@]} == 0)); then
  echo "No Teensies found."
else
  echo "Teensies found: ${#teensies[@]}"
  for i in "${!teensies[@]}"; do
    echo "Uploading to Teensy $((i + 1)) of ${#teensies[@]}."
    tycmd upload firmware.hex -B "${teensies[$i]}"
  done
  echo "Done!"
fi
