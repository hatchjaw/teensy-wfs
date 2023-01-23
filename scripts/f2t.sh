#!/bin/bash
# Wrapper for faust2teensy
file=$1
faustFlags=${@:2}

if [ -z "$file" ]; then
  echo "Expected to receive a .dsp file"
fi

# Move to directory holding the dsp file.
dir=$(dirname "$(realpath "$1")")
cd "$dir" || exit 1

if [[ $file == *.dsp ]]; then
  name=$(basename "$file" .dsp)
  echo "Generating faust object."
  # Faust object needs UI macros for FAUST_INPUTS, etc.;
  faust2teensy -lib $name.dsp $faustFlags
  cd ..
  echo "Extracting archive to $(pwd)/$name"
  unzip -qo "$dir/$name".zip
  echo "Tidying up."
  rm "$dir/$name".zip
  echo "Done"
else
  echo "Usage: faust2teensy.sh [filename].dsp"
fi
