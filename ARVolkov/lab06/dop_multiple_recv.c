#include <mpi.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if(size < 2) {
        if (rank == 0)
            printf("Need at least 2 processes\n");
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
        for (int i = 1; i < size; i++) {
            
            printf("Process 0: sending message to process %d\n", i);
            MPI_Send(message, strlen(message) + 1, MPI_CHAR, i, tag_message, MPI_COMM_WORLD);
        }
        

        for (int i = 1; i < size; i++) {

            printf("Getting answer from process %d\n", i);
            MPI_Recv(reply, sizeof(reply), MPI_CHAR, i, tag_reply, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Got answer from process %d\nReply: %s\n", i, reply);
        }
    } else {
        printf("Getting message from process %d\n", 0);
        MPI_Recv(message, sizeof(message), MPI_CHAR, 0, tag_message, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Got message from process %d\nMsg: %s\n", 0, message);

        
        snprintf(reply, sizeof(reply), "Back hello from process %d", rank);
        MPI_Send(reply, strlen(reply) + 1, MPI_CHAR, 0, tag_reply, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}