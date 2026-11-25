#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

int* fetch_nums(char* buffer, int buff_bound) {
    // вся строка должна быть вида "<полож число> <полож число>" без каких либо символов в конце
    if (buffer == NULL || buff_bound <= 0) return NULL;

    int i = 0;
    long long n1 = 0, n2 = 0;

    // Первое число - строго начинается с цифры, никаких пробелов в начале
    if (buffer[0] < '0' || buffer[0] > '9') return NULL;
    
    while (i <= buff_bound && buffer[i] >= '0' && buffer[i] <= '9') {
        n1 = n1 * 10 + (buffer[i] - '0');
        if (n1 > INT_MAX) return NULL; // Защита от переполнения int
        i++;
    }

    printf("[строка %d] Первое число прочитали\n", __LINE__);

    // Ровно один пробел между числами
    if (i > buff_bound || buffer[i] != ' ') return NULL;
    i++; 

    printf("[строка %d] Пробел пропустили прочитали\n", __LINE__);
    
    // что граница буфера еще не достигнута
    if (i > buff_bound) return NULL;

    // Второе число - строго начинается с цифры
    if (buffer[i] < '0' || buffer[i] > '9') return NULL;

    while (i <= buff_bound && buffer[i] >= '0' && buffer[i] <= '9') {
        n2 = n2 * 10 + (buffer[i] - '0');
        if (n2 > 2147483647) return NULL;
        i++;
    }

    printf("[строка %d] Второе число прочитали\n", __LINE__);

    // если конец строки не достигнут значит строка не соответствует шаблону
    if (i <= buff_bound) return NULL;
    
    printf("[строка %d] Успех\n", __LINE__);

    // успех, выделяем память и возвращаем
    int* result = (int*)malloc(2 * sizeof(int));
    if (result == NULL) return NULL;

    result[0] = (int)n1;
    result[1] = (int)n2;
    
    return result;
}