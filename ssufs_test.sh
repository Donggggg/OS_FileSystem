#!/bin/bash

gcc -o ssufs_test ssufs_test2.c ssufs-ops.c ssufs-disk.c
./ssufs_test

rm -f ssufs
rm -f ssufs_test

