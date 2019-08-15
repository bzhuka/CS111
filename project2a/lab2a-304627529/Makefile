#!/bin/bash

main: lab2_add.c
	gcc -o lab2_add -g -pthread -Wall lab2_add.c
	gcc -o lab2_list -g -pthread -Wall lab2_list.c SortedList.c

clean:
	rm lab2_add
	rm lab2_list
	rm lab2_add.csv
	rm lab2_list.csv
	rm lab2_add-?.png
	rm lab2_list-?.png

tests:
	./testing

graphs:
	/u/cs/grad/zhou/iloveos/gnuplot lab2_add.gp lab2_list.gp

tarball:
	tar -czvf lab2a-304627529.tar.gz lab2_add.c SortedList.h SortedList.c lab2_list.c lab2_add.gp lab2_list.gp Makefile lab2_add.csv lab2_list.csv testing lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png README.txt 
