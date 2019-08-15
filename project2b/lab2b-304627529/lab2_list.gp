#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab_2b_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2b_1.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#

# general plot parameters
set terminal png
set datafile separator ","

# lab2b_1.png

set title "Scalability-1: Synchronized Throughput"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_1.png'
set key left top
plot \
     "< grep add-m pic12.csv" using ($2):(1000000000/($6)) \
	title 'adds w/mutex' with linespoints lc rgb 'green', \
     "< grep add-s pic12.csv" using ($2):(1000000000/($6)) \
	title 'adds w/spin' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[^,]*,1000,1,' pic12.csv" using ($2):(1000000000/($7)) \
	title 'list ins/lookup/delete w/mutex' with linespoints lc rgb 'orange', \
     "< grep 'list-none-s,[^,]*,1000,1,' pic12.csv" using ($2):(1000000000/($7)) \
	title 'list ins/lookup/delete w/spin' with linespoints lc rgb 'violet'

# lab2b_2.png

set title "Scalability-2: Per-operation Times for List Operations"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "mean time/operation (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key left top
plot \
     "< grep 'list-none-m,[^,]*,1000,1,' pic12.csv" using ($2):($7) \
	title 'completion time' with linespoints lc rgb 'orange', \
     "< grep 'list-none-m,[^,]*,1000,1,' pic12.csv" using ($2):($8) \
	title 'wait for lock' with linespoints lc rgb 'green'

# lab2b_3.png
set title "Scalability-3: Correct Synchronization of Partitioned Lists"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
set key left top
plot \
     "< grep 'list-id-none,[^,]*,[^,]*,4,' lab_2b_list.csv" using ($2):($3) \
	title 'yield=id' with points lc rgb "red", \
     "< grep 'list-id-m,[^,]*,[^,]*,4,' lab_2b_list.csv" using ($2):($3) \
	title 'Mutex' with points lc rgb "green", \
     "< grep 'list-id-s,[^,]*,[^,]*,4,' lab_2b_list.csv" using ($2):($3) \
	title 'Spin-Lock' with points lc rgb "blue"

# lab2b_4.png
set title "Scalability-4: Mutex-Synchronized Throughput of Partitioned Lists"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange[0.75:]
set ylabel "Throughput(operations/sec)"
set logscale y 10
set output 'lab2b_4.png'
set key left top
plot \
     "< grep 'list-none-m,[^,]*,1000,1,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb "purple", \
     "< grep 'list-none-m,[^,]*,1000,4,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb "green", \
     "< grep 'list-none-m,[^,]*,1000,8,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb "blue", \
     "< grep 'list-none-m,[^,]*,1000,16,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=16' with linespoints lc rgb "orange", \


# lab2b_5.png
set title "Scalability-5: Spin-Lock-Synchronized Throughput of Partitioned Lists"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange[0.75:]
set ylabel "Throughput(operations/sec)"
set logscale y 10
set output 'lab2b_5.png'
set key left top
plot \
     "< grep 'list-none-s,[^,]*,1000,1,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb "purple", \
     "< grep 'list-none-s,[^,]*,1000,4,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb "green", \
     "< grep 'list-none-s,[^,]*,1000,8,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb "blue", \
     "< grep 'list-none-s,[^,]*,1000,16,' lab_2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=16' with linespoints lc rgb "orange", \
