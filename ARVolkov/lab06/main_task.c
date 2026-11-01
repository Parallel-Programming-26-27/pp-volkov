#include <mpi.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size != 2) {
        if (rank == 0)
            printf("Need only 2 processes\n");
        MPI_Finalize();
        return 0;
    }

    
    int tag_message = 0;
    int tag_reply = 1;
    //тк разные процессы данные неразделяемые
    char message[100];
    char reply[100];

    if(rank == 0) {
        snprintf(message, sizeof(message), "Hello from main worker");
        
        printf("Process 0: sending message to process 1\n");
        MPI_Send(message, strlen(message) + 1, MPI_CHAR, 1, tag_message, MPI_COMM_WORLD);
        
        

        printf("Getting answer from process 1\n");
        MPI_Recv(reply, sizeof(reply), MPI_CHAR, 1, tag_reply, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Got answer from process 1\nReply: %s\n", reply);
        
    } else {
        printf("Getting message from process 0\n");
        MPI_Recv(message, sizeof(message), MPI_CHAR, 0, tag_message, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Got message from process 0\nMsg: %s\n", message);

        
        snprintf(reply, sizeof(reply), "back hello from process %d", rank);
        MPI_Send(reply, strlen(reply) + 1, MPI_CHAR, 0, tag_reply, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}