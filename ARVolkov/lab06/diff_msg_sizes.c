#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_SIZE 1048576

//с memset memcpy невозможно работать
void build_message(char *buf, int size, int rank) {
    char suffix[64];
    snprintf(suffix, sizeof(suffix), "msg from proc %d", rank);
    int suffix_len = (int)strlen(suffix) + 1;

    if (suffix_len > size) {
        for (int i = 0; i < size; i++) buf[i] = 'x';
        return;
    }

    int fill_len = size - suffix_len;

    for (int i = 0; i < fill_len; i++) {
        buf[i] = 'x';
    }

    snprintf(buf + fill_len, size - fill_len, "msg from proc %d", rank);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size != 2) {
        if (rank == 0) printf("Need 2 processes\n");
        MPI_Finalize();
        return 0;
    }

    char *sbuf = malloc(MAX_SIZE);
    char *rbuf = malloc(MAX_SIZE);

    int sizes[] = {1, 10, 100, 10000, 65536, 1048576};
    int n = sizeof(sizes) / sizeof(sizes[0]);


    int tag_message = 0;
    int tag_reply = 1;

    for (int i = 0; i < n; i++) {
        int count = sizes[i];
        build_message(sbuf, count, rank);

        // число повторений для замера
        int reps = 10000;
        if (count >= 10000)  reps = 1000;
        if (count >= 100000) reps = 100;

        double t0 = 0.0;
        if (rank == 0) t0 = MPI_Wtime();

        for (int k = 0; k < reps; k++) {
            if (rank == 0) {
                MPI_Send(sbuf, count, MPI_CHAR, 1, tag_message, MPI_COMM_WORLD);
                MPI_Recv(rbuf, count, MPI_CHAR, 1, tag_reply, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(rbuf, count, MPI_CHAR, 0, tag_message, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(rbuf, count, MPI_CHAR, 0, tag_reply, MPI_COMM_WORLD);
            }
        }

        if (rank == 0) {
            double t1 = MPI_Wtime(); // Стоп таймера
            double avg = (t1 - t0) / reps;
            printf("size=%8d bytes, reps=%5d, avg RTT=%.9f s\n", count, reps, avg);
        }
    }

    

    free(sbuf);
    free(rbuf);

    MPI_Finalize();
    return 0;
}


