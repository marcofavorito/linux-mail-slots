#!/usr/bin/env bash
TEST_NAME="one_thread_rw";
SIZES="0008 0016 0032 0064 0128 0256 0512 1024 2048 4096 8192"
#SIZES="8191"

NUM_OPS=10000000;

for i in $SIZES; do
	sudo ./mailslot_load
	echo "dataunit size: $i"

	echo "$TEST_EXE_DIR/a.out 1 1 $NUM_OPS $NUM_OPS $DEFAULT_BLOCKING_WRITE $DEFAULT_BLOCKING_READ 1 $i $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME}_$i.dat"
	sudo $TEST_EXE_DIR/a.out 1 1 $NUM_OPS $NUM_OPS $DEFAULT_BLOCKING_WRITE $DEFAULT_BLOCKING_READ 1 $i $DEFAULT_MAX_STORAGE  > $EVAL_DIR/${TEST_NAME}_$i.dat

	sudo ./mailslot_unload;
done;

gnuplot -persist <<- EOF
	SIZES="0008 0016 0032 0064 0128 0256 0512 1024 2048 4096 8192"
#	SIZES="8191"
	set terminal pdf

    set output '${EVAL_DIR}/${TEST_NAME}.pdf'
    set title "One writer/One reader"
    plot for [i=1:words(SIZES)] "${EVAL_DIR}/${TEST_NAME}_"  .word(SIZES, i).".dat" using 1:2 with lines lc i title 's=writer-'.word(SIZES,i).'', \
         for [i=1:words(SIZES)] "${EVAL_DIR}/${TEST_NAME}_"  .word(SIZES, i).".dat" using 1:3 with lines lc i dt 2 title 's=reader-'.word(SIZES,i).''

	#do for [i=1:words(SIZES)] {}

    pause -1;
EOF