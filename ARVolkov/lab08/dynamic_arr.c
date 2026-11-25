#include <stdio.h>
#include <stdlib.h>
#include "dynamic_arr.h"


// Инициализация массива (начальная вместимость)
void init_array(DynamicArray* arr, int initial_capacity) {
    arr->data = (int*)malloc(initial_capacity * sizeof(int));
    if (arr->data == NULL) {
        printf("Ошибка выделения памяти!\n");
        exit(1);
    }
    arr->size = 0;
    arr->capacity = initial_capacity;
}

// Добавление элемента в конец (автоматически расширяет память)
void push_back(DynamicArray* arr, int value) {
    if (arr->size == arr->capacity) {
        arr->capacity *= 2; // Удваиваем вместимость
        arr->data = (int*)realloc(arr->data, arr->capacity * sizeof(int));
        if (arr->data == NULL) {
            printf("Ошибка realloc!\n");
            exit(1);
        }
    }
    arr->data[arr->size] = value;
    arr->size++;
}

// Очистка (освобождение памяти)
void free_array(DynamicArray* arr) {
    free(arr->data);
    arr->data = NULL;
    arr->size = 0;
    arr->capacity = 0;
}

// Очистка только содержимого (для переиспользования в следующем уровне BFS)
void clear_array(DynamicArray* arr) {
    arr->size = 0;
}