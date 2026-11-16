#!/bin/sh

FOLDERS="genome intruder kmeans labyrinth ssca2 vacation yada "

rm lib/*.o || true

rm lib/tm.h
rm common/Defines.common.mk
rm common/Makefile
rm lib/thread.h
rm lib/thread.c

for F in $FOLDERS
do
    cd $F
    rm *.o || true
    rm $F
    cd ..
done
