#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi.h>
#include <stddef.h>
#include "Messages.h"

#define CLIENT 0
#define PRIMARY 1
#define MPI_REQUEST_TAG INT_MAX

MPI_Status status;   
int my_rank, p;
MPI_Datatype mpi_request;
MPI_Datatype mpi_pre_prepare;
MPI_Datatype mpi_prepare;
MPI_Datatype mpi_commit;
MPI_Datatype mpi_reply;


void client(){
    printf("Sou o cliente\n");  
    
    request r;
    r.timestamp = MPI_Wtime();
    r.request_type = ADD;
    
    MPI_Send(&r, 1, mpi_request, PRIMARY, MPI_REQUEST_TAG, MPI_COMM_WORLD);
    
    //todo: aguardar respostas
}
void primary(){
    
    printf("Sou o líder\n");
    
    request r;
    MPI_Recv(&r, 1, mpi_request, CLIENT, MPI_REQUEST_TAG, MPI_COMM_WORLD, &status);
    
    printf("[%d].Request timestamp:%f type:%d",my_rank, r.timestamp, r.request_type);
      
    pre_prepare pp;
    pp.view = 1;
    pp.sequence_number = 1; //todo: incrementar
    pp.request_type = r.request_type;
    pp.process_id = my_rank;

    
    for(int i = 0; i < p; i++){
            MPI_Send(&pp, 1, mpi_pre_prepare, i, pp.sequence_number, MPI_COMM_WORLD);
            MPI_Send(&r, 1, mpi_request, i, pp.sequence_number, MPI_COMM_WORLD);
    }
    
    printf("[&d].Pre-prepare enviado.", my_rank);
    
    //todo: esperar predicado prepared()
    
    
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
    
    //Create request mpi type
    MPI_Aint request_displacements[2] = { 
        offsetof(request, timestamp),
        offsetof(request, request_type)
    };
    
    int request_block_length[2] = {1, 1};
    MPI_Datatype request_types[2] = {MPI_DOUBLE, MPI_INT};
    
    MPI_Type_create_struct(2, request_block_length, request_displacements, request_types, &mpi_request);
    
    //Create reply mpi type
    MPI_Aint reply_displacements[4] = { 
        offsetof(reply, view),
        offsetof(reply, process_id),
        offsetof(reply, timestamp),
        offsetof(reply, result)
    };
    
    int reply_block_length[4] = {1, 1, 1, 1};
    MPI_Datatype reply_types[4] = {MPI_INT, MPI_INT, MPI_DOUBLE,              MPI_INT};
    
    MPI_Type_create_struct(4, reply_block_length, reply_displacements, reply_types, &mpi_request);
    
    
    //Create pre-prepare mpi type
    MPI_Type_contiguous(4, MPI_INT, &mpi_pre_prepare);
    MPI_Type_commit(&mpi_pre_prepare);
    
    //Create prepare mpi type
    MPI_Type_contiguous(4, MPI_INT, &mpi_prepare);
    MPI_Type_commit(&mpi_prepare);
    
    //Create commit mpi type
    MPI_Type_contiguous(4, MPI_INT, &mpi_commit);
    MPI_Type_commit(&mpi_commit);
    
    if(my_rank == 0){
        client();
    }else if(my_rank == 1){
        primary();
    }else{
        replica();
    }
    
    
    MPI_Finalize();   
}