#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi.h>
#include "Messages.h"

MPI_Status status;   
int my_rank, p;
MPI_Datatype mpi_pre_prepare;


void client(){
    printf("Sou o cliente\n");  
    pre_prepare pp;
    pp.view = 1;
    pp.sequence_number = 1;
    pp.request_type = ADD;
    pp.process_id = my_rank;
    
    MPI_Send(&pp, 1, mpi_pre_prepare, 1, pp.sequence_number, MPI_COMM_WORLD);
}
void primary(){
    printf("Sou o líder\n");
    pre_prepare pp;
    
    MPI_Status status;
    //MPI_Datatype mpi_pre_prepare;
    
    MPI_Recv(&pp, 1, mpi_pre_prepare, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    
    printf("Recebido de %d\n", pp.process_id);
}
void replica(){
    printf("Sou a replica %d\n", my_rank);
}

int main(int argc, char** argv){
    
    //request *r = (request*)malloc(sizeof(request));
    
    
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    
    //Create pre-prepare mpi type
    MPI_Type_contiguous(4, MPI_INT, &mpi_pre_prepare);
    MPI_Type_commit(&mpi_pre_prepare);
    
    if(my_rank == 0){
        client();
    }else if(my_rank == 1){
        primary();
    }else{
        replica();
    }
    
    
    MPI_Finalize();   
}