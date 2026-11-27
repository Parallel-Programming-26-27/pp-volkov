#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <mpi.h>
#include <omp.h>
#include "bfs_lib.h"
#include "dynamic_arr.h"


int main(int argc, char** argv) {
    const char* graph_file = argc > 1 ? argv[1] : "graph.txt";
    graph* concrete_graph = create_graph_from_file(graph_file);
    if (concrete_graph == NULL) {
        printf("Something went wrong\n");
        return -1;
    }

    int rank;  
    int nprocs;  


    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    // Каждый процесс обрабатывает свою часть текущего фронта.
    // Списки кандидатов потоков объединяются в local_candidates,
    // затем MPI_Allgatherv собирает их в global_candidates.
    // После обмена каждый процесс обновляет dist и строит следующий фронт.
    const int start_node = 1;

    int* dist = malloc(concrete_graph->node_cnt * sizeof(int));
    if (dist == NULL) {
        printf("Ошибка выделения памяти для dist\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    //очередь из связного списка
    linked_node* start_q = NULL;
    linked_node* end_q = NULL;
    linked_node* curr_n = NULL;

    int queue_size = 0;
    int level = 0;

    int max_threads = omp_get_max_threads();
    DynamicArray* thread_candidates = malloc(max_threads * sizeof(DynamicArray));
    if (thread_candidates == NULL) {
        printf("Ошибка выделения памяти для списков потоков\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int i = 0; i < max_threads; i++) {
        init_array(&thread_candidates[i], 16);
    }

    DynamicArray local_candidates;
    init_array(&local_candidates, 16);

    double best = 1e30;

    for (int r = 0; r < 7; r++) {
        for (int i = 0; i < concrete_graph->node_cnt; i++) {
            dist[i] = -1;
        }
        dist[start_node] = 0;

        start_q = create_q_node(start_node);
        if (start_q == NULL) {
            printf("Ошибка выделения памяти для начальной вершины\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        end_q = start_q;
        queue_size = 1;
        level = 0;

        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();

        while (start_q != NULL) {
            //queue_size - количество вершин которые надо обработать
            int* queue_vertexes = malloc(queue_size * sizeof(int));

            curr_n = start_q;

            for (int i = 0; i < queue_size; i++) {
                queue_vertexes[i] = curr_n->graph_node_num;

                linked_node* trash_node = curr_n;
                curr_n = curr_n -> next;
                free(trash_node);
            }

            start_q = NULL;
            end_q = NULL;

            //теперь распределение вершин по процессам
            int curr_vertx_num = queue_size / nprocs;
            if (rank <= (queue_size % nprocs) - 1) {
                curr_vertx_num++;
            }

            clear_array(&local_candidates);
            for (int i = 0; i < max_threads; i++) {
                clear_array(&thread_candidates[i]);
            }

            #pragma omp parallel
            {
                int tid = omp_get_thread_num();

                #pragma omp for schedule(dynamic, 1)
                for (int i = 0; i < curr_vertx_num; i++) {
                    int curr_node = queue_vertexes[rank + i * nprocs];
                    graph_node* node = &concrete_graph->nodes[curr_node];

                    for (int j = 0; j < node->connected_nodes_cnt; j++) {
                        int candidate = node->connected_nodes_nums[j];

                        if (dist[candidate] == -1) {
                            push_back(&thread_candidates[tid], candidate);
                        }
                    }
                }
            }

            for (int tid = 0; tid < max_threads; tid++) {
                for (int i = 0; i < thread_candidates[tid].size; i++) {
                    push_back(&local_candidates, thread_candidates[tid].data[i]);
                }
            }

            free(queue_vertexes);

            int recvcounts[nprocs];
            int displs[nprocs];

            MPI_Allgather(
                &local_candidates.size, 1, MPI_INT,
                recvcounts, 1, MPI_INT,
                MPI_COMM_WORLD
            );

            int global_candidates_size = 0;
            for (int i = 0; i < nprocs; i++) {
                displs[i] = global_candidates_size;
                global_candidates_size += recvcounts[i];
            }

            int* global_candidates = malloc(
                (global_candidates_size > 0 ? global_candidates_size : 1) * sizeof(int)
            );
            if (global_candidates == NULL) {
                printf("Ошибка выделения памяти для global_candidates\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            MPI_Allgatherv(
                local_candidates.data, local_candidates.size, MPI_INT,
                global_candidates, recvcounts, displs, MPI_INT,
                MPI_COMM_WORLD
            );

            int next_queue_size = 0;
            for (int i = 0; i < global_candidates_size; i++) {
                int candidate = global_candidates[i];

                if (dist[candidate] == -1) {
                    dist[candidate] = level + 1;

                    linked_node* new_node = create_q_node(candidate);
                    if (new_node == NULL) {
                        printf("Ошибка выделения памяти для вершины следующего фронта\n");
                        MPI_Abort(MPI_COMM_WORLD, 1);
                    }

                    if (start_q == NULL) {
                        start_q = new_node;
                        end_q = new_node;
                    }
                    else {
                        end_q->next = new_node;
                        end_q = new_node;
                    }
                    next_queue_size++;
                }
            }

            free(global_candidates);
            queue_size = next_queue_size;
            level++;
        }

        double t = MPI_Wtime() - t0;
        double max_t;
        MPI_Reduce(&t, &max_t, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        if (rank == 0 && r > 0 && max_t < best) {
            best = max_t;
        }
    }

    for (int i = 0; i < max_threads; i++) {
        free_array(&thread_candidates[i]);
    }
    free(thread_candidates);
    free_array(&local_candidates);

    if (rank == 0) {
        int* sequential_dist = bfs_func(
            start_node, concrete_graph->nodes, concrete_graph->node_cnt
        );
        if (sequential_dist == NULL) {
            printf("Ошибка последовательного обхода графа\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        int unreachable_count = 0;
        int mismatch_count = 0;
        long edge_count = 0;

        for (int i = 0; i < concrete_graph->node_cnt; i++) {
            int sequential_unreachable = sequential_dist[i] == INT_MAX;
            int parallel_unreachable = dist[i] == -1;

            if (parallel_unreachable) {
                unreachable_count++;
            }

            if (sequential_unreachable != parallel_unreachable ||
                (!sequential_unreachable && !parallel_unreachable &&
                 sequential_dist[i] != dist[i])) {
                mismatch_count++;
            }

            edge_count += concrete_graph->nodes[i].connected_nodes_cnt;
        }
        edge_count /= 2;

        printf("Время параллельного BFS: %.3f ms\n", best * 1e3);
        printf("\nПроверка корректности по пункту 3.1:\n");
        printf("Файл графа: %s\n", graph_file);
        printf("Число вершин: %d\n", concrete_graph->node_cnt);
        printf("Число рёбер: %ld\n", edge_count);
        printf("Начальная вершина: %d\n", start_node);
        printf("Число уровней: %d\n", level);
        printf("Число недостижимых вершин: %d\n", unreachable_count);
        printf("Число несовпадающих расстояний: %d\n", mismatch_count);
        printf("Результат сравнения: %s\n",
               mismatch_count == 0 ? "расстояния совпадают" : "обнаружены различия");

        printf("\nДистанции от вершины %d:\n", start_node);
        for (int i = 0; i < concrete_graph->node_cnt; i++) {
            if (dist[i] == -1) {
                printf("вершина %d: INF\n", i);
            } else {
                printf("вершина %d: %d\n", i, dist[i]);
            }
        }

        free(sequential_dist);
    }

    free(dist);

    //очистка памяти от связного списка
    //TODO сделать
    
    //очистка памяти от графа
    for (int i = 0; i < concrete_graph->node_cnt; i++) {
        free(concrete_graph->nodes[i].connected_nodes_nums);
    }
    free(concrete_graph->nodes);
    free(concrete_graph);

    MPI_Finalize();

    return 0;
}