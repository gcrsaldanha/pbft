#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi.h>
#include <stddef.h>
#include <limits.h>
#include "Messages.h"

#define CLIENT 0
#define PRIMARY 1
#define MPI_REQUEST_TAG INT_MAX

int my_rank, p, view = 1;
int f;
int state = 0;
MPI_Status status;
MPI_Datatype mpi_request;
MPI_Datatype mpi_pre_prepare;
MPI_Datatype mpi_prepare;
MPI_Datatype mpi_commit;
MPI_Datatype mpi_reply;

//caso 5 nós
int faulty_nodes[6] = {0, 0, 0, 0, 0, 0};
//int faulty_nodes[6] = {0, 0, 1, 0, 0, 0};
//int faulty_nodes[6] = {0, 0, 1, 1, 0, 0}; //Caso de falha
//int faulty_nodes[6] = {0, 0, 1, 1, 1, 1}; //Caso de falha

//caso 10 nós
//int faulty_nodes[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//int faulty_nodes[11] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
//int faulty_nodes[11] = {0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0};
//int faulty_nodes[11] = {0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0}; //Caso de falha
//int faulty_nodes[11] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1}; //Caso de falha

//caso 20 nós
//int faulty_nodes[21] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//int faulty_nodes[21] = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//int faulty_nodes[21] = {0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//int faulty_nodes[21] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //Caso de falha
//int faulty_nodes[21] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; //Caso de falha


int prepared(request req, int sequence_number)
{
    int received_prepares[p - 1];
    int i;

    prepare prep;

    for (i = 0; i < p - 1; i++)
        received_prepares[i] = -1;
    received_prepares[my_rank - 1] = 1;

    for (i = 2; i < p; i++)
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
    //printf("[%d].Counter: %d", my_rank, counter);
    return counter >= 2 * f;
}

int commited_local(pre_prepare p_prep, request req)
{
    int received_commits[p - 1];
    int i;

    commit commit;

    for (i = 0; i < p - 1; i++)
        received_commits[i] = -1;
    received_commits[my_rank - 1] = 1;

    for (i = 1; i < p - 1; i++)
    {
        if (i != my_rank)
        {
            MPI_Recv(&commit, 1, mpi_commit, i, p_prep.sequence_number, MPI_COMM_WORLD, &status);
            //printf("[%d].Commit recebido de %d", my_rank, i);

            received_commits[i - 1] = (commit.process_id == status.MPI_SOURCE && commit.view == view && commit.sequence_number == p_prep.sequence_number && commit.request_type == p_prep.request_type);
        }
    }

    int counter = 0;

    for (i = 0; i < p - 1; i++)
    {
        if (received_commits[i] == 1)
            counter++;
    }
    //printf("[%d].Counter: %d", my_rank, counter);
    return counter >= 2 * f + 1;
}

void execute(request req)
{
    switch (req.request_type)
    {
    case ADD:
        state++;
        break;
    case SUB:
        state--;
        break;
    default:
        break;
    }
}

void client()
{
    //printf("Sou o cliente\n");

    request req;
    req.timestamp = MPI_Wtime();
    req.request_type = ADD;

    MPI_Send(&req, 1, mpi_request, PRIMARY, MPI_REQUEST_TAG, MPI_COMM_WORLD);

    //todo: aguardar respostas
    int replies[p - 1];

    for (int i = 0; i < p - 1; i++)
    {
        replies[i] = INT_MIN;
    }

    reply reply;

    for (int i = 1; i < p; i++)
    {
        MPI_Recv(&reply, 1, mpi_reply, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        if (!req.timestamp == reply.timestamp || !reply.process_id == status.MPI_SOURCE || reply.view == -1)
        {
            replies[i - 1] = INT_MIN;
        }
        else
        {
            replies[i - 1] = reply.result;
        }
    }

    //Verificação do resultado (f+1)

    int candidate;
    int counter;
    for (int i = 0; i < p - 1; i++)
    {
        candidate = replies[i];
        counter = 0;
        for (int j = i; j < p - 1; j++)
        {
            if (candidate == replies[j] && candidate != INT_MIN)
            {
                counter++;
            }
        }
        if (counter >= f + 1)
        {
            printf("[%d].Resultado: %d.\n", my_rank, candidate);
            return;
        }
    }

    printf("[%d].Consenso não alcançado!", my_rank);
}
void primary()
{
    //todo: implementar read
    //printf("Sou o líder\n");

    int is_faulty = 0;
    request req;
    MPI_Recv(&req, 1, mpi_request, CLIENT, MPI_REQUEST_TAG, MPI_COMM_WORLD, &status);

    printf("[%d].Request timestamp:%f type:%d\n", my_rank, req.timestamp, req.request_type);

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

    printf("[%d].Pre-prepare enviado.\n", my_rank);
    printf("[%d].Pre-prepare adcionado ao log.\n", my_rank);

    //Predicado prepared()
    if (!prepared(req, sequence_number))
        is_faulty = 1;

    if (!is_faulty)
    {
        printf("[%d].Preparado.\n", my_rank);
    }

    commit commit;

    if (is_faulty)
    {
        commit.view = -1;
    }
    else
    {
        commit.view = view;
    }

    commit.sequence_number = sequence_number;
    commit.request_type = req.request_type;
    commit.process_id = PRIMARY;

    for (int i = 2; i < p; i++)
    {
        MPI_Send(&commit, 1, mpi_commit, i, sequence_number, MPI_COMM_WORLD);
    }

    if (!is_faulty)
    {
        printf("[%d].Commit enviado.\n", my_rank);
    }

    //Predicado commited-local
    if (!commited_local(p_prep, req))
        is_faulty = 1;

    reply reply;

    if (is_faulty)
    {
        reply.view = -1;
        reply.result = INT_MIN;
    }
    else
    {
        execute(req);
        reply.view = view;
        reply.result = state;
    }

    reply.process_id = my_rank;
    reply.timestamp = req.timestamp;

    MPI_Send(&reply, 1, mpi_reply, CLIENT, sequence_number, MPI_COMM_WORLD);

    if (!is_faulty)
    {
        printf("[%d].Reply enviado.\n", my_rank);
    }
}
void replica()
{
    //printf("Sou a replica %d\n", my_rank);

    int is_faulty = faulty_nodes[my_rank];

    if (is_faulty)
    {
        printf("[%d]. Sou defeituoso!\n", my_rank);
    }

    pre_prepare p_prep;
    MPI_Recv(&p_prep, 1, mpi_pre_prepare, PRIMARY, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    int sequence_number = status.MPI_TAG;

    // Verifica se o número de sequência da requisição é igual ao do pre-prepare
    if (p_prep.process_id != status.MPI_SOURCE || p_prep.sequence_number != sequence_number || p_prep.view != view)
        is_faulty = 1;

    request req;
    MPI_Recv(&req, 1, mpi_request, PRIMARY, sequence_number, MPI_COMM_WORLD, &status);

    //Verifica se o pre-prepare corresponde ao request e se a assinuta é válida
    if (p_prep.process_id != status.MPI_SOURCE || req.request_type != p_prep.request_type)
        is_faulty = 1;

    if (!is_faulty)
    {
        printf("[%d].Pre-prepare aceito.\n", my_rank);
    }

    prepare prep;

    if (is_faulty)
    {
        prep.view = -1;
    }
    else
    {
        prep.view = view;
    }

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

    if (!is_faulty)
    {
        printf("[%d].Prepare enviado.\n", my_rank);
        printf("[%d].Pre-prepare adcionado ao log.\n", my_rank);
        printf("[%d].Prepare adcionado ao log.\n", my_rank);
    }

    if (!prepared(req, sequence_number))
        is_faulty = 1;

    if (!is_faulty)
    {
        printf("[%d].Preparado.\n", my_rank);
    }

    commit commit;
    if (is_faulty)
    {
        commit.view = -1;
    }
    else
    {
        commit.view = view;
    }

    commit.sequence_number = sequence_number;
    commit.request_type = req.request_type;
    commit.process_id = my_rank;

    for (int i = 1; i < p; i++)
    {
        if (i != my_rank)
        {
            MPI_Send(&commit, 1, mpi_commit, i, sequence_number, MPI_COMM_WORLD);
        }
    }

    if (!is_faulty)
    {
        printf("[%d].Commit enviado.\n", my_rank);
    }

    //Predicado commited-local
    if (!commited_local(p_prep, req))
        is_faulty = 1;

    reply reply;

    if (is_faulty)
    {
        reply.view = -1;
        reply.result = INT_MIN;
    }
    else
    {
        execute(req);
        reply.view = view;
        reply.result = state;
    }

    reply.process_id = my_rank;
    reply.timestamp = req.timestamp;

    MPI_Send(&reply, 1, mpi_reply, CLIENT, sequence_number, MPI_COMM_WORLD);

    if (!is_faulty)
    {
        printf("[%d].Reply enviado.\n", my_rank);
    }
}

int main(int argc, char **argv)
{
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
    MPI_Type_commit(&mpi_request);

    //Create reply mpi type
    MPI_Aint reply_displacements[4] = {
        offsetof(reply, view),
        offsetof(reply, process_id),
        offsetof(reply, timestamp),
        offsetof(reply, result)};

    int reply_block_length[4] = {1, 1, 1, 1};
    MPI_Datatype reply_types[4] = {MPI_INT, MPI_INT, MPI_DOUBLE, MPI_INT};

    MPI_Type_create_struct(4, reply_block_length, reply_displacements, reply_types, &mpi_reply);
    MPI_Type_commit(&mpi_reply);

    //Create pre-prepare mpi type
    MPI_Type_contiguous(4, MPI_INT, &mpi_pre_prepare);
    MPI_Type_commit(&mpi_pre_prepare);

    //Create prepare mpi type
    MPI_Type_contiguous(4, MPI_INT, &mpi_prepare);
    MPI_Type_commit(&mpi_prepare);

    //Create commit mpi type
    MPI_Type_contiguous(4, MPI_INT, &mpi_commit);
    MPI_Type_commit(&mpi_commit);

    /*
    FILE *log_file;
    char file_path[20]; 
    sprintf(file_path, "./logs/log_%d.txt", my_rank);
    printf("%s\n", file_path);
    log_file = fopen(file_path, "w+"); 
    */
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
    
    //fclose(log_file);

    MPI_Finalize();
}