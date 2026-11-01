#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#ifndef ELEM
#define ELEM double 
#endif
typedef ELEM T;
 
#define RUNS 5 /* число измеряемых запусков (берётся медиана) */
 

typedef void (*transpose_fn)(const T *A, T *B, long n, long b);
 
/* Один блок [ii, ii+b) x [jj, jj+b); границы обрезаются, если n не кратно b. */
static inline void tile(const T *A, T *B, long n, long ii, long jj, long b) {
    long ie = ii + b < n ? ii + b : n;
    long je = jj + b < n ? jj + b : n;
    for (long i = ii; i < ie; i++)
        for (long j = jj; j < je; j++)
            B[j * n + i] = A[i * n + j];
}
 
/* Внешний цикл по строкам A: чтение подряд, запись в B с шагом n. */
static void transpose_rows(const T *A, T *B, long n, long b) {
    for (long i = 0; i < n; i++)
        for (long j = 0; j < n; j++)
            B[j * n + i] = A[i * n + j];
}
 
/* Внешний цикл по столбцам A: запись подряд, чтение из A с шагом n. */
static void transpose_cols(const T *A, T *B, long n, long b) {
    for (long j = 0; j < n; j++)
        for (long i = 0; i < n; i++)
            B[j * n + i] = A[i * n + j];
}
 
static void transpose_blocked(const T *A, T *B, long n, long b) {
    for (long ii = 0; ii < n; ii += b)
        for (long jj = 0; jj < n; jj += b)
            tile(A, B, n, ii, jj, b);
}
 

static void transpose_rows_omp(const T *A, T *B, long n, long b) {
    #pragma omp parallel for schedule(runtime)
    for (long i = 0; i < n; i++)
        for (long j = 0; j < n; j++)
            B[j * n + i] = A[i * n + j];
}
 
/* Распараллеливание по строкам блоков. */
static void transpose_blocked_omp(const T *A, T *B, long n, long b) {
    #pragma omp parallel for
    for (long ii = 0; ii < n; ii += b)
        for (long jj = 0; jj < n; jj += b)
            tile(A, B, n, ii, jj, b);
}
 
static int cmp_double(const void *x, const void *y) {
    double a = *(const double *)x, c = *(const double *)y;
    return (a > c) - (a < c);
}
 
int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Использование: %s N b потоки версия [chunk]\n"
                        "версии: rows cols blocked rows_omp blocked_omp\n", argv[0]);
        return 1;
    }
    long n = atol(argv[1]), b = atol(argv[2]);
    int threads = atoi(argv[3]);
    const char *name = argv[4];
    int chunk = argc > 5 ? atoi(argv[5]) : 0;
 
    static const struct { const char *name; transpose_fn f; int parallel; } V[] = {
        {"rows", transpose_rows, 0},
        {"cols", transpose_cols, 0},
        {"blocked", transpose_blocked, 0},
        {"rows_omp", transpose_rows_omp, 1},
        {"blocked_omp", transpose_blocked_omp, 1},
    };
    int v = -1;
    for (int k = 0; k < 5; k++)
        if (!strcmp(name, V[k].name)) v = k;
    if (n <= 0 || b <= 0 || threads <= 0 || chunk < 0 || v < 0) {
        fprintf(stderr, "Неверные параметры\n");
        return 1;
    }
 
    if (!V[v].parallel) threads = 1; /* последовательные версии всегда в один поток */
    omp_set_num_threads(threads);
    omp_set_schedule(omp_sched_static, chunk); /* chunk = 0 -> обычный schedule(static) */
 
    size_t cnt = (size_t)n * n;
    T *A = malloc(cnt * sizeof(T));
    T *B = malloc(cnt * sizeof(T));
    T *R = malloc(cnt * sizeof(T)); /* эталон: результат transpose_rows */
    if (!A || !B || !R) {
        fprintf(stderr, "Не хватило памяти\n");
        return 1;
    }
    for (size_t k = 0; k < cnt; k++) A[k] = (T)(k % 100003);
    transpose_rows(A, R, n, b);
 
    double times[RUNS];
    long bad = 0; /* число несовпадений с эталоном по всем запускам */
    for (int r = -1; r < RUNS; r++) { /* r = -1: прогревочный запуск, не учитывается */
        memset(B, 0, cnt * sizeof(T)); /* чтобы проверка видела результат именно этого запуска */
        double t0 = omp_get_wtime();
        V[v].f(A, B, n, b);
        double t = omp_get_wtime() - t0;
        if (r >= 0) times[r] = t;
        for (size_t k = 0; k < cnt; k++) bad += (B[k] != R[k]);
    }
    qsort(times, RUNS, sizeof(double), cmp_double);
    double t = times[RUNS / 2]; /* медиана */
 
    printf("N=%ld b=%ld threads=%d version=%s time=%.6f s BW=%.2f GB/s ",
           n, b, threads, name, t, 2.0 * n * n * sizeof(T) / t / 1e9);
    if (bad) printf("check=FAIL (несовпадений: %ld)\n", bad);
    else printf("check=OK\n");
 
    free(A); free(B); free(R);
    return bad != 0;
}