#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <mpi.h>
#include <omp.h>
#include "bfs_lib.h"
#include "dynamic_arr.h"


int main(int argc, char** argv) {
    //сразу ищем расстояния от 0-ой вершины
    //парсим граф
    graph* concrete_graph = create_graph_from_file("graph.txt");
    if (concrete_graph == NULL) {
        printf("Something went wrong\n");
        return -1;
    }
    show_graph(concrete_graph);  
    
    int rank;  
    int nprocs;  


    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    // то есть алгоритм в main между инит
    // и финалайз для каждого процесса: 
    // сначала создаем global_candidate с нулями,
    // инициализируем очередь с начальной вершиной,
    // делаем цикл while очередь не пуста, вытаскиваем 
    // с массив все вершины из очереди(опустошаем ее),
    // подсчитываем текущий фронт для своего  процесса
    // создаем local_candidates, каждый процесс ее подсчитывает-
    // ставит флаг в той ячейке номер которой принадлежит
    // непосещенной вершине(dist[i] = -1) и потом объединяем все локал
    // кандидейтс в один, обновляем global_candidates и результат кладем
    // в очередь(только объединенный локал кандидейтс не глобал) и по новой
    int* dist = (int*)calloc(concrete_graph->node_cnt, sizeof(int)); 
    for (int i = 1; i < concrete_graph->node_cnt; i++) {
        dist[i] = -1;
    }
    
    //очередь из связного списка
    linked_node* start_q = NULL;
    linked_node* end_q = NULL;

    linked_node* curr_n = create_q_node(start_node);
    start_q = curr_n;
    end_q = curr_n;
    int queue_size = 1;

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
    }

    for (int i = 0; i < max_threads; i++) {
        free_array(&thread_candidates[i]);
    }
    free(thread_candidates);
    free_array(&local_candidates);

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