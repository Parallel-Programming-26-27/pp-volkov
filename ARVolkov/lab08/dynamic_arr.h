// Структура динамического массива
typedef struct {
    int* data;
    int size;
    int capacity;
} DynamicArray;

// Инициализация массива (начальная вместимость)
void init_array(DynamicArray* arr, int initial_capacity);
// Добавление элемента в конец (автоматически расширяет память)
void push_back(DynamicArray* arr, int value);
// Очистка (освобождение памяти)
void free_array(DynamicArray* arr);
// Очистка только содержимого (для переиспользования в следующем уровне BFS)
void clear_array(DynamicArray* arr);