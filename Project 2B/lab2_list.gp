#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2b_1.png ... throughput vs. number of threads
#	lab2b_2.png ... mean time per mutex wait and mean time per operation
#	lab2b_3.png ... successful iterations vs. threads
#	lab2b_4.png ... throughput vs. number of threads
#	lab2b_5.png ... throughput vs. number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

# throughput vs. number of threads for mutex and spin-lock synchronized list operations.
set title "List-1: Throughput vs. Number of Threads"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operation/ns)"
set logscale y 10
set output 'lab2b_1.png'

# grep out only protected, non-yield results
plot \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'mutex' with linespoints lc rgb 'red', \
	 "< grep 'list-none-s,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'spin-lock' with linespoints lc rgb 'blue'


# wait-for-lock time and average time per operation vs. number of threads for mutex
set title "List-2: Time per Op vs. Number of Threads"
set xlabel "Number of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Average Time/Operation (ns)"
set logscale y 10
set output 'lab2b_2.png'

# grep out only mutex, non-yield results
plot \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'Wait-for-Lock Time' with linespoints lc rgb 'red', \
	 "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'Total Time' with linespoints lc rgb 'blue'

# throughput vs number of threads for mutex
set title "List-4: Throughput vs. Number of Threads for mutex"
set xlabel "Threads"
set ylabel "Throughput (operations/second)"
set logscale x 2
set logscale y
unset xrange
set xrange [0.75:]
unset yrange
set output 'lab2b_4.png'
plot \
     "< grep 'list-none-m,[0-9][2]\\?,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list=1' with linespoints lc rgb 'red', \
	 "< grep 'list-none-m,[0-9][2]\\?,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list=4' with linespoints lc rgb 'blue', \
	 "< grep 'list-none-m,[0-9][2]\\?,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list=8' with linespoints lc rgb 'green', \
	 "< grep 'list-none-m,[0-9][2]\\?,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list=16' with linespoints lc rgb 'pink'

# throughput vs number of threads for spin-lock
set title "List-5: Throughput vs. Number of Threads for spin-lock"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations/s)"
set logscale y
set output 'lab2b_5.png'
# grep out only mutex, non-yield results
plot \
     "< grep 'list-none-s,[0-9][2]\\?,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list=1' with linespoints lc rgb 'red', \
	 "< grep 'list-none-s,[0-9][2]\\?,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list=4' with linespoints lc rgb 'blue', \
	 "< grep 'list-none-s,[0-9][2]\\?,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list=8' with linespoints lc rgb 'green', \
	 "< grep 'list-none-s,[0-9][2]\\?,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list=16' with linespoints lc rgb 'pink'

# grep out only --lists=4, --yield=id results
set title "List-3: Iterations that run without failure"
set xlabel "Threads"
set xrange [0.75:17]
set ylabel "successful iterations"
set logscale y 10
set yrange [0.75:100]
set output 'lab2b_3.png'
plot \
     "< grep 'list-id-none,.*,4,' lab2b_list.csv" using ($2):($3) \
	title 'unprotected' with points lc rgb 'red', \
     "< grep 'list-id-s,.*,4,' lab2b_list.csv" using ($2):($3) \
	title 'spin-lock' with points lc rgb 'blue', \
     "< grep 'list-id-m,.*,4,' lab2b_list.csv" using ($2):($3) \
	title 'mutex' with points lc rgb 'green'
