#!/usr/bin/env bash
TEST_NAME="more_threads_rw"
NO_THREADS="00002 00005 00010 00050 00100 01000"
#NO_THREADS="00100"
NUMS_OPS="10000000 1000000 1000000 100000 100000 10000"
#NUMS_OPS="100000"

no_threads=( $NO_THREADS )
nums_ops=( $NUMS_OPS )

for ((i=0; i<${#no_threads[@]}; i++))
do
	sudo ./mailslot_load;
	cur_no_threads=${no_threads[i]}
	cur_num_ops=${nums_ops[i]}
	echo "number of threads: $cur_no_threads, number of operations: $cur_num_ops"

	echo "sudo $TEST_EXE_DIR/a.out $cur_no_threads $cur_no_threads $cur_num_ops $cur_num_ops $DEFAULT_BLOCKING_WRITE $DEFAULT_BLOCKING_READ 1 $DEFAULT_MAX_DATAUNIT_LEN $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME}_$cur_no_threads.dat"
	sudo $TEST_EXE_DIR/a.out $cur_no_threads $cur_no_threads $cur_num_ops $cur_num_ops $DEFAULT_BLOCKING_WRITE $DEFAULT_BLOCKING_READ 1 $DEFAULT_MAX_DATAUNIT_LEN $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME}_$cur_no_threads.dat

	sudo ./mailslot_unload;
done;

gnuplot -persist <<- EOF
	NO_THREADS="00002 00005 00010 00050 00100 01000"
#	NO_THREADS="00100"
	NUMS_OPS="10000000 1000000 1000000 100000 100000 10000"
#	NUMS_OPS="100000"
	set terminal pdf

    set output '${EVAL_DIR}/${TEST_NAME}.pdf';
    set title "More writers/More readers"
    plot for [i=1:words(NO_THREADS)] "${EVAL_DIR}/${TEST_NAME}_"  .word(NO_THREADS, i).".dat" using 1:2 with lines lc i title 's=writer-'.word(NO_THREADS,i).'',\
         for [i=1:words(NO_THREADS)] "${EVAL_DIR}/${TEST_NAME}_"  .word(NO_THREADS, i).".dat" using 1:3 with lines lc i dt 2 title 's=reader-'.word(NO_THREADS,i).''

	#do for [i=1:words(NO_WRITERS)] {}

    pause -1;
EOF