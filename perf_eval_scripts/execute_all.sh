#!/usr/bin/env bash
export PROJECT_DIR=.
export TEST_DIR=$PROJECT_DIR/test
export EVAL_DIR=$PROJECT_DIR/perf_eval_scripts
export TEST_EXE_DIR=$PROJECT_DIR/test/testStress

export DEFAULT_MAX_STORAGE=12000000000
export DEFAULT_MAX_DATAUNIT_LEN=32
export DEFAULT_BLOCKING_WRITE=1
export DEFAULT_BLOCKING_READ=1

rm $EVAL_DIR/*.dat $EVAL_DIR/*.pdf
gcc $TEST_EXE_DIR/thread.c -lpthread -o $TEST_EXE_DIR/a.out

echo "***************************"
echo "$(date)" | tee -a log
echo "***************************"

$EVAL_DIR/one_thread.sh                     | tee -a $EVAL_DIR/log
$EVAL_DIR/more_threads.sh                   | tee -a $EVAL_DIR/log
$EVAL_DIR/one_thread_rw.sh                  | tee -a $EVAL_DIR/log
$EVAL_DIR/more_threads_rw.sh                | tee -a $EVAL_DIR/log
$EVAL_DIR/nonblocking_one_thread.sh         | tee -a $EVAL_DIR/log
$EVAL_DIR/nonblocking_more_threads.sh       | tee -a $EVAL_DIR/log
$EVAL_DIR/nonblocking_one_thread_rw.sh      | tee -a $EVAL_DIR/log
$EVAL_DIR/nonblocking_more_threads_rw.sh    | tee -a $EVAL_DIR/log
