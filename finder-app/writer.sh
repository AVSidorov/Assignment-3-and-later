#!/bin/sh
# Author Anton Sidorov

#set -e

writefile=$1
writestr=$2

if [ $# -lt 2 ]
then
  echo "Not enough arguments. It's required 2"
  exit 1
fi

writedir=$(dirname $writefile)

if !(mkdir -p $writedir)
then
  echo "Failed create directory"
  exit 1
fi

if !(echo $writestr > $writefile)
then
  echo "Failed create file"
  exit 1
fi
