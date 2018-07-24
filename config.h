#ifndef CONFIG_H
#define CONFIG_H

//#define DEBUG 1 //0=false  1=true  | enable debug print

// NOT CHANGE!
#define SUCCESS 0         // const
#define ERROR -1          // const
//#define MAX_INSTANCES 256 // const

//Upper and lower bounds for dataunit size and storage size
const int MIN_STORAGE_SIZE  = 0;
const int MIN_DATAUNIT_SIZE =   0;
//const int MAX_STORAGE_SIZE  = 2<<14; //16KB
//const int MAX_DATAUNIT_SIZE =   128;
const int MAX_STORAGE_SIZE  = 1<<31;
const int MAX_DATAUNIT_SIZE =   8192;

#endif
