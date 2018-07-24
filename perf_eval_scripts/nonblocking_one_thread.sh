#!/usr/bin/env bash
TEST_NAME="nonblocking_one_thread_writer";
TEST_NAME_2="nonblocking_one_thread_reader";
SIZES="0008 0016 0032 0064 0128 0256 0512 1024 2048 4096 8192"
#SIZES="8191"

NUM_OPS=1000000000;

for i in $SIZES; do
	sudo ./mailslot_load
	echo "dataunit size: $i"

	echo "sudo $TEST_EXE_DIR/a.out 1 0 $NUM_OPS 0 0 0 1 $i $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME}_$i.dat"
	sudo $TEST_EXE_DIR/a.out 1 0 $NUM_OPS 0 0 0 1 $i $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME}_$i.dat

	echo "sudo $TEST_EXE_DIR/a.out 0 1 0 $NUM_OPS 0 0 1 $i $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME_2}_$i.dat"
	sudo $TEST_EXE_DIR/a.out 0 1 0 $NUM_OPS 0 0 1 $i $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME_2}_$i.dat

	sudo ./mailslot_unload;
done;

gnuplot -persist <<- EOF
	SIZES="0008 0016 0032 0064 0128 0256 0512 1024 2048 4096 8192"
#	SIZES="8191"
	set terminal pdf

    set output '${EVAL_DIR}/${TEST_NAME}.pdf'
    set title "One writer (non-blocking)"
    plot for [i=1:words(SIZES)] "${EVAL_DIR}/${TEST_NAME}_"  .word(SIZES, i).".dat" using 1:2 with lines title 's='.word(SIZES,i).''

    set output '${EVAL_DIR}/${TEST_NAME_2}.pdf'
    set title "One reader (non-blocking)"
    plot for [i=1:words(SIZES)] "${EVAL_DIR}/${TEST_NAME_2}_".word(SIZES, i).".dat" using 1:3 with lines title 's='.word(SIZES,i).''

	#do for [i=1:words(SIZES)] {}

    pause -1;
EOF
