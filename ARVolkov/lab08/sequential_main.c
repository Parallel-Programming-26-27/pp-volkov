#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <omp.h>
#include "bfs_lib.h"


int main(int argc, char** argv) {
    const char* graph_file = argc > 1 ? argv[1] : "graph.txt";
    graph* concrete_graph = create_graph_from_file(graph_file);
    if (concrete_graph == NULL) {
        printf("Something went wrong\n");
        return -1;
    }

    const int start_node = 1;
    long edge_count = 0;
    for (int i = 0; i < concrete_graph->node_cnt; i++) {
        edge_count += concrete_graph->nodes[i].connected_nodes_cnt;
    }
    edge_count /= 2;

    int* dist = NULL;
    double best = 1e30;

    for (int r = 0; r < 7; r++) {
        double t0 = omp_get_wtime();
        dist = bfs_func(start_node, concrete_graph->nodes, concrete_graph->node_cnt);
        double t = omp_get_wtime() - t0;

        if (dist == NULL) {
            break;
        }
        if (r > 0 && t < best) {
            best = t;
        }
        if (r < 6) {
            free(dist);
            dist = NULL;
        }
    }

    if (dist == NULL) {
        printf("Ошибка обхода графа в ширину\n");
    } else {
        int level_count = 0;
        int unreachable_count = 0;

        for (int i = 0; i < concrete_graph->node_cnt; i++) {
            if (dist[i] == INT_MAX) {
                unreachable_count++;
            } else if (dist[i] + 1 > level_count) {
                level_count = dist[i] + 1;
            }
        }

        printf("Статистика последовательного BFS:\n");
        printf("Файл графа: %s\n", graph_file);
        printf("Число вершин: %d\n", concrete_graph->node_cnt);
        printf("Число рёбер: %ld\n", edge_count);
        printf("Начальная вершина: %d\n", start_node);
        printf("Число уровней: %d\n", level_count);
        printf("Число недостижимых вершин: %d\n", unreachable_count);
        printf("Время последовательного BFS: %.3f ms\n", best * 1e3);

        printf("\nДистанции от вершины %d:\n", start_node);
        for (int i = 0; i < concrete_graph->node_cnt; i++) {
            if (dist[i] == INT_MAX) {
                printf("вершина %d: INF\n", i);
            } else {
                printf("вершина %d: %d\n", i, dist[i]);
            }
        }

        free(dist);
    }

    //очистка
    for (int i = 0; i < concrete_graph->node_cnt; i++) {
        free(concrete_graph->nodes[i].connected_nodes_nums);
    }
    free(concrete_graph->nodes);
    free(concrete_graph);
    
    return 0;
}