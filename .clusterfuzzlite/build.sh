#!/bin/bash -eu
$CC $CFLAGS -c -Icorestack/include -Icorestack/include/lib -Itetris/include -Itetris/include/lib \
  corestack/src/lib/libhypertext.c -o $WORK/libhypertext.o
$CC $CFLAGS -c -Icorestack/include -Icorestack/include/lib -Itetris/include -Itetris/include/lib \
  corestack/src/lib/libhtttp.c -o $WORK/libhtttp.o
$CC $CFLAGS -c -Icorestack/include -Icorestack/include/lib -Itetris/include -Itetris/include/lib \
  corestack/src/lib/libhtttp/payload.c -o $WORK/payload.o
$CC $CFLAGS -c -Icorestack/include -Icorestack/include/lib -Itetris/include -Itetris/include/lib \
  tetris/tests/fuzz/fuzz_htttp.c -o $WORK/fuzz_htttp.o

$CXX $CXXFLAGS $WORK/fuzz_htttp.o $WORK/libhypertext.o $WORK/libhtttp.o $WORK/payload.o \
  $LIB_FUZZING_ENGINE -o $OUT/fuzz_htttp