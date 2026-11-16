#!/bin/bash

Benches="genome intruder kmeans labyrinth ssca2 vacation yada" # bayes

if [[ $# != 1 ]]; then
  echo "./allbuild.sh <tm>"
  exit 1
fi

backend=${1}
if [ ! -d ./${stm} ]; then
  echo "${stm} does not exist!"
  exit 1
fi


cp backends/${backend}/tm.h lib/tm.h
cp backends/${backend}/Makefile common/Makefile
cp backends/${backend}/Defines.common.mk common/Defines.common.mk

echo -n "Building ${backend} ... "
cd ${backend}
make clean >&/dev/null
make >&/dev/null
if [[ $? != 0 ]]; then
  echo -e "\nBuild error : ${backend}"
  rm common/Makefile common/Defines.common.mk
  rm lib/*.o lib/tm.h
  exit 1
fi
cd ..
echo "done"

for bench in ${Benches}; do
  echo -n "  Compiling ${bench} ... "
  (
    cd ${bench}
    make -f Makefile 1> /dev/null
    rc=$?
    if [[ $rc != 0 ]] ; then
      echo -e "\nBuild error : ${bench}"
      rm *.o ${bench} || true
      exit 1
    fi

    dst=${backend}
    mkdir -p "../build/${dst}"
    if [[ -f ../build/${dst}/${bench} ]]; then
      rm -f ../build/${dst}/${bench} >&/dev/null
    fi
    cp ./${bench} ../build/${dst}

    rm -f *.o ${bench} || true >&/dev/null
  )
  echo "done"
done

rm common/Makefile common/Defines.common.mk
rm lib/*.o lib/tm.h
