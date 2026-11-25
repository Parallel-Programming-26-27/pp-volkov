#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <mpi.h>
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

        int* vertx_for_proc = malloc(curr_vertx_num * sizeof(int));

        for (int i = 0; i < curr_vertx_num; i++) {
            int curr_node = queue_vertexes[rank + i * (nprocs)];
        }

        
    }
    

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