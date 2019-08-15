#!/bin/bash

override CFLAGS += -pthread -g

build: lab2_add.c
	gcc -o lab2_add -g -pthread -Wall lab2_add.c
	gcc -o lab2_list -g -pthread -Wall lab2_list.c SortedList.c

lab2_list: lab2_list.o SortedList.o
	gcc -o $@ -pthread lab2_list.o SortedList.o

profile: lab2_list Makefile
	-rm -f ./raw.gperf
	LD_PRELOAD=/u/cs/ugrad/bradleyz/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile
	pprof --list=fThreads ./lab2_list ./raw.gperf >> $@
	rm -f ./raw.gperf

clean:
	rm lab2_add
	rm lab2_list
	rm pic12.csv
	rm lab_2b_list.csv
	rm lab2b_?.png
	rm profile

tests:
	./test

graphs:
	/u/cs/grad/zhou/iloveos/gnuplot lab2_list.gp

tarball:
	tar -czvf lab2b-304627529.tar.gz lab2_add.c SortedList.h SortedList.c lab2_list.c lab2_list.gp Makefile pic12.csv lab_2b_list.csv test lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png README.txt 
