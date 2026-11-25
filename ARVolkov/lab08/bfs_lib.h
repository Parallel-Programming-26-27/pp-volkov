//нумерацию графов будем начинать с 0

//структура ноды, для списков смежности
typedef struct {
    int node_num;
    int connected_nodes_cnt;
    //пока длина массива будет по умолчанию 20
    int* connected_nodes_nums;
} graph_node;

//структура графа
typedef struct {
    int node_cnt;
    graph_node* nodes;
} graph;


int* bfs_func(int start_node, graph_node* graph, int graph_nodes_cnt);
graph* create_graph_from_file(char* file_name);
void cleanup_mmap_fd(char* data, size_t file_size, int fd);
void show_graph(graph* concrete_graph);


//связной список для очереди
typedef struct linked_node {
    struct linked_node* next;
    //если что номер вершины
    int graph_node_num;
} linked_node;

linked_node* create_q_node(int node_num);