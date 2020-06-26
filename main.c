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
int my_rank, p, view = 1;
int f;
MPI_Datatype mpi_request;
MPI_Datatype mpi_pre_prepare;
MPI_Datatype mpi_prepare;
MPI_Datatype mpi_commit;
MPI_Datatype mpi_reply;

int prepared(request req, int sequence_number)
{
    int received_prepares[p - 1];
    int i;

    prepare prep;

    for (i = 0; i < p - 1; i++)
        received_prepares[i] = -1;
    received_prepares[my_rank - 1] = 1;

    for (i = 1; i < p; i++)
    {
        if (i != my_rank)
        {
            MPI_Recv(&prep, 1, mpi_prepare, i, sequence_number, MPI_COMM_WORLD, &status);

            received_prepares[i - 1] = (prep.process_id == status.MPI_SOURCE && prep.view == view && prep.sequence_number == sequence_number);
        }
    }

    int counter = 0;

    for (i = 0; i < p - 1; i++)
    {
        if (received_prepares[i] == 1)
            counter++;
    }

    return counter == 2 * f + 1;
}
void client()
{
    printf("Sou o cliente\n");

    request req;
    req.timestamp = MPI_Wtime();
    req.request_type = ADD;

    MPI_Send(&req, 1, mpi_request, PRIMARY, MPI_REQUEST_TAG, MPI_COMM_WORLD);

    //todo: aguardar respostas
}
void primary()
{

    printf("Sou o líder\n");

    request req;
    MPI_Recv(&req, 1, mpi_request, CLIENT, MPI_REQUEST_TAG, MPI_COMM_WORLD, &status);

    printf("[%d].Request timestamp:%f type:%d", my_rank, req.timestamp, req.request_type);

    int sequence_number = 1;

    pre_prepare p_prep;
    p_prep.view = 1;
    p_prep.sequence_number = sequence_number;
    p_prep.request_type = req.request_type;
    p_prep.process_id = my_rank;

    for (int i = 2; i < p; i++)
    {
        //Envio do Pre-prepare
        MPI_Send(&p_prep, 1, mpi_pre_prepare, i, sequence_number, MPI_COMM_WORLD);
        //Envio da mensagem request
        MPI_Send(&req, 1, mpi_request, i, sequence_number, MPI_COMM_WORLD);
    }

    printf("[&d].Pre-prepare enviado.\n", my_rank);
    printf("[&d].Pre-prepare adcionado ao log.\n", my_rank);

    //Predicado prepared()
    if (!prepared(req, sequence_number))
        return;
    /*Return não é a melhor solução.
      Usar MPI_ABORT?
      int MPI_Abort(MPI_Comm comm, int errorcode)
    */
}
void replica()
{
    //printf("Sou a replica %d\n", my_rank);

    pre_prepare p_prep;
    MPI_Recv(&p_prep, 1, mpi_pre_prepare, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    int sequence_number = status.MPI_TAG;

    // Verifica se o número de sequência da requisiçõa é igual ao do pre-prepare
    if (p_prep.process_id != status.MPI_SOURCE)
        return;
    if (p_prep.sequence_number != sequence_number)
        return;
    if (p_prep.view != view)
        return;

    request req;
    MPI_Recv(&req, 1, mpi_request, MPI_ANY_SOURCE, sequence_number, MPI_COMM_WORLD, &status);
    if (p_prep.process_id != status.MPI_SOURCE)
        return;
    if (req.request_type != p_prep.request_type)
        return;

    printf("[%d]. Pre-prepare aceito.\n", my_rank);

    prepare prep;
    prep.view = view;
    prep.sequence_number = sequence_number;
    prep.request_type = req.request_type;
    prep.process_id = my_rank;

    for (int i = 1; i < p; i++)
    {
        if (i != my_rank)
        {
            MPI_Send(&prep, 1, mpi_prepare, i, sequence_number, MPI_COMM_WORLD);
        }
    }
    printf("[%d]. Prepare enviado.\n", my_rank);
    printf("[%d]. Pre-prepare adcionado ao log.\n", my_rank);
    printf("[%d]. Prepare adcionado ao log.\n", my_rank);

    if (!prepared(req, sequence_number))
        return;
}

int main(int argc, char **argv)
{

    //request *req = (request*)malloc(sizeof(request));

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    f = (p - 2) / 3; // p-2 para pq o cliente é um processo

    //Create request mpi type
    MPI_Aint request_displacements[2] = {
        offsetof(request, timestamp),
        offsetof(request, request_type)};

    int request_block_length[2] = {1, 1};
    MPI_Datatype request_types[2] = {MPI_DOUBLE, MPI_INT};

    MPI_Type_create_struct(2, request_block_length, request_displacements, request_types, &mpi_request);

    //Create reply mpi type
    MPI_Aint reply_displacements[4] = {
        offsetof(reply, view),
        offsetof(reply, process_id),
        offsetof(reply, timestamp),
        offsetof(reply, result)};

    int reply_block_length[4] = {1, 1, 1, 1};
    MPI_Datatype reply_types[4] = {MPI_INT, MPI_INT, MPI_DOUBLE, MPI_INT};

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

    if (my_rank == 0)
    {
        client();
    }
    else if (my_rank == 1)
    {
        primary();
    }
    else
    {
        replica();
    }

    MPI_Finalize();
}