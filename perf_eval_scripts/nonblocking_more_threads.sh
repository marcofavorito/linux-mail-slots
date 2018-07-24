#!/usr/bin/env bash
TEST_NAME="nonblocking_more_threads_writer";
TEST_NAME_2="nonblocking_more_threads_reader";
NO_THREADS="00002 00005 00010 00050 00100 01000"
#NO_THREADS="10000"

NUM_OPS=1000000000;

for i in $NO_THREADS; do
	sudo ./mailslot_load;
	echo "number of threads: $i"

	echo "sudo $TEST_EXE_DIR/a.out $i 0 $NUM_OPS 0 0 0 1 $DEFAULT_MAX_DATAUNIT_LEN $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME}_$i.dat"
	sudo $TEST_EXE_DIR/a.out $i 0 $NUM_OPS 0 0 0 1 $DEFAULT_MAX_DATAUNIT_LEN $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME}_$i.dat

	echo "sudo $TEST_EXE_DIR/a.out 0 $i 0 $NUM_OPS 0 0 1 $DEFAULT_MAX_DATAUNIT_LEN $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME_2}_$i.dat"
	sudo $TEST_EXE_DIR/a.out 0 $i 0 $NUM_OPS 0 0 1 $DEFAULT_MAX_DATAUNIT_LEN $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME_2}_$i.dat

	sudo ./mailslot_unload;
done;

gnuplot -persist <<- EOF
	NO_THREADS="00002 00005 00010 00050 00100 01000"
#	NO_THREADS="10000"
	set terminal pdf

    set output '${EVAL_DIR}/${TEST_NAME}.pdf';
    set title "More writers (non-blocking)"
    plot for [i=1:words(NO_THREADS)] "${EVAL_DIR}/${TEST_NAME}_".word(NO_THREADS, i).".dat" with lines title 's='.word(NO_THREADS,i).''

    set output '${EVAL_DIR}/${TEST_NAME_2}.pdf'
    set title "More readers (non-blocking)"
    plot for [i=1:words(NO_THREADS)] "${EVAL_DIR}/${TEST_NAME_2}_".word(NO_THREADS, i).".dat" using 1:3 with lines title 's='.word(NO_THREADS,i).''

	#do for [i=1:words(NO_WRITERS)] {}

    pause -1;
EOF