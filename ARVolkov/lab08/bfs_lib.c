#include <stdlib.h>
#include <limits.h>
#include "bfs_lib.h"
#include "helper.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>

//нумерацию графов будем начинать с 0


linked_node* create_q_node(int node_num) {
    linked_node* new_node = malloc(sizeof(linked_node));
    if (new_node != NULL) {
        new_node->next = NULL;
        new_node->graph_node_num = node_num;
    }
    return new_node;
}

//функция обхода графа в ширину
//возвращает массив дистанций
//будет возвращать null если что то пошло не так
int* bfs_func(int start_node, graph_node* graph, int graph_nodes_cnt) {
    int* dist = malloc(graph_nodes_cnt * sizeof(int));

    if (dist == NULL) {
        printf("[строка %d] Ошибка выделения памяти в bfs_func\n", __LINE__);
        return NULL;
    }
    for (int i = 0; i < graph_nodes_cnt; i++) {
        if (i == start_node) {
            *(dist + i) = 0;
        } else {
            *(dist + i) = INT_MAX;
        }
    }

    linked_node* start_q = NULL;
    linked_node* end_q = NULL;

    linked_node* curr_n = create_q_node(start_node);
    

    start_q = curr_n;
    end_q = curr_n;

    while (start_q != NULL) {
        curr_n = start_q;

        start_q = start_q -> next;
        if (start_q == NULL)
            end_q = NULL;
        
        graph_node node = *(graph + curr_n -> graph_node_num);
        int* neighbours = node.connected_nodes_nums;

        for (int i = 0; i < node.connected_nodes_cnt; i++) {
            int neighbour_node_num = *(neighbours + i);

            if (dist[curr_n -> graph_node_num] + 1 < dist[neighbour_node_num]) {
                dist[neighbour_node_num] = dist[curr_n -> graph_node_num] + 1;

                linked_node* new_n = create_q_node(neighbour_node_num);
                
                if (start_q == NULL) {
                    start_q = new_n;
                    end_q = start_q;
                } else {
                    end_q -> next = new_n;
                    end_q = new_n;
                }
            }
        }
        
        free(curr_n);
    }


    return dist;
}

//-----
//извлечение графа из файла
//формат файла:
//кол-во_вершин кол-во_ребер
//потом кол-во_ребер строк в виде "<номер вершины> <номер вершины>"
//-----
//при любой ошибке при выделении памяти 
//или неправильном формате файла графа
//функция вернет NULL
//-----
graph* create_graph_from_file(const char* file_name) {
    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        printf("[строка %d] something went wrong while opening file\n", __LINE__);
        return NULL;
    }

    struct stat sb;
    fstat(fd, &sb);

    size_t file_size = sb.st_size;

    char* data = mmap(
        NULL,
        file_size,
        PROT_READ,
        MAP_PRIVATE,
        fd,
        0
    );

    if (data == MAP_FAILED) {
        printf("[строка %d] something went wrong while mapping file\n", __LINE__);
        cleanup_mmap_fd(data, file_size, fd);
        return NULL;
    }

    char buffer[256];
    int buff_bound = -1;

    size_t i = 0;

    bool header_read = false;
    while (!header_read) {
        char curr = *(data + i);

        if (curr == '\n') {
            header_read = true;
        }
        else {
            buff_bound++;
            buffer[buff_bound] = *(data + i);
        }
        
        i++;
    }

    int v_num;//количество вершин графа
    int* nums = fetch_nums(buffer, buff_bound);
    if (nums == NULL) {
        printf("[строка %d] Неверный формат строк\n", __LINE__);
        cleanup_mmap_fd(data, file_size, fd);
        return NULL;
    }
    else {
        v_num = nums[0];
    }
    free(nums);

    //собираем структуры нод
    graph_node* concrete_graph_nodes = malloc(v_num * sizeof(graph_node));
    if (concrete_graph_nodes == NULL) {
        printf("[строка %d] Ошибка выделения памяти\n", __LINE__);
        cleanup_mmap_fd(data, file_size, fd);
        return NULL;
    }

    for (int g_i = 0; g_i < v_num; g_i++) {
        concrete_graph_nodes[g_i].node_num = g_i;

        int* connected_nodes_arr = malloc(20 * sizeof(int));
        if (connected_nodes_arr == NULL) {
            printf("[строка %d] Ошибка выделения памяти для connected_nodes_arr\n", __LINE__);
            for (int g_j = 0; g_j < g_i; g_j++) {
                free(concrete_graph_nodes[g_j].connected_nodes_nums);
            }
            free(concrete_graph_nodes);
            cleanup_mmap_fd(data, file_size, fd);
            return NULL;
        }

        concrete_graph_nodes[g_i].connected_nodes_nums = connected_nodes_arr;
        concrete_graph_nodes[g_i].connected_nodes_cnt = 0;
    }


    buff_bound = -1;
    while (i <= file_size) {

        char curr = *(data + i);

        if (curr == '\n' || i == file_size) {
            
            nums = fetch_nums(buffer, buff_bound);
            if (nums == NULL) {
                printf("[строка %d] Неверный формат строк\n", __LINE__);
                for (int g_j = 0; g_j < v_num; g_j++) {
                    free(concrete_graph_nodes[g_j].connected_nodes_nums);
                }
                free(concrete_graph_nodes);
                cleanup_mmap_fd(data, file_size, fd);
                return NULL;
            }
            
            int n1 = nums[0];
            int n2 = nums[1];

            concrete_graph_nodes[n1].connected_nodes_nums[
                concrete_graph_nodes[n1].connected_nodes_cnt
            ] = n2;
            concrete_graph_nodes[n1].connected_nodes_cnt++;

            concrete_graph_nodes[n2].connected_nodes_nums[
                concrete_graph_nodes[n2].connected_nodes_cnt
            ] = n1;
            concrete_graph_nodes[n2].connected_nodes_cnt++;

            buff_bound = -1;
            free(nums);
        }
        else {
            buff_bound++;
            buffer[buff_bound] = *(data + i);
        }
        
        i++;
    }

    graph* concrete_graph = malloc(sizeof(graph));

    concrete_graph->nodes = concrete_graph_nodes;
    concrete_graph->node_cnt = v_num;

    munmap(data, file_size);
    close(fd);

    return concrete_graph;
}
