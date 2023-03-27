#!/bin/sh
# Author Anton Sidorov

set -e

filesdir=$1
searchstr=$2


if [ $# -lt 2 ]
then
  echo "Not enough arguments. It's required 2"
  exit 1
elif [ ! -d $filesdir ]
then
    echo "${filesdir} not a dir or not exist"
    exit 1
else
  out=$(grep -F --count -h -r $searchstr $filesdir/* 2> /dev/null)
  #nfile=$(grep -F --count -h -r $searchstr $filesdir/* 2> /dev/null | wc -l)

  nline=0

  for line in $out
  do
      nfile=$(( nfile + 1)) # sh don't have ++ operator
      nline=$(( nline + line ))
  done

  echo "The number of files are ${nfile} and the number of matching lines are ${nline}"
fi
