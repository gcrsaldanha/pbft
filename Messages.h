#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define ADD 1
#define SUB 2



typedef struct Request{
    double timestamp;
    int request_type;
}request;

typedef struct Pre_Prepare{
    int view;
    int sequence_number;
    int request_type;
    int process_id;
}pre_prepare;

typedef struct Prepare{
    int view;
    int sequence_number;
    int request_type;
    int process_id;
}prepare;

typedef struct Commit{
    int view;
    int sequence_number;
    int request_type;
    int process_id;
}commit;

typedef struct Reply{
    int view;
    int process_id;
    double timestamp;
    int result;
}reply;