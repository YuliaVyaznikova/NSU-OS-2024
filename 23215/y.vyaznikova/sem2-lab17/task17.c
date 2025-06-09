#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_STRING_LENGTH 80
#define SORT_INTERVAL 5

typedef struct node {
    char *data;
    struct node *next;
} node;

typedef struct {
    node *head;
    pthread_mutex_t mutex;
} list;

list string_list;
int should_exit = 0;

void list_init(list *l) {
    l->head = NULL;
    pthread_mutex_init(&l->mutex, NULL);
}

void list_destroy(list *l) {
    pthread_mutex_lock(&l->mutex);
    node *current = l->head;
    while (current != NULL) {
        node *next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
    pthread_mutex_unlock(&l->mutex);
    pthread_mutex_destroy(&l->mutex);
}

void list_insert(list *l, const char *str) {
    node *new_node = malloc(sizeof(node));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    new_node->data = strdup(str);
    if (new_node->data == NULL) {
        free(new_node);
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    pthread_mutex_lock(&l->mutex);
    new_node->next = l->head;
    l->head = new_node;
    pthread_mutex_unlock(&l->mutex);
}

void list_print(list *l) {
    pthread_mutex_lock(&l->mutex);
    node *current = l->head;
    printf("Current list contents:\n");
    while (current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }
    printf("End of list\n");
    pthread_mutex_unlock(&l->mutex);
}

void bubble_sort(list *l) {
    pthread_mutex_lock(&l->mutex);
    
    if (l->head == NULL || l->head->next == NULL) {
        pthread_mutex_unlock(&l->mutex);
        return;
    }

    int swapped;
    node *ptr1;
    node *lptr = NULL;

    swapped = 1;
    while (swapped) {
        swapped = 0;
        ptr1 = l->head;

        while (ptr1->next != lptr) {
            if (strcmp(ptr1->data, ptr1->next->data) > 0) {
                char *temp = ptr1->data;
                ptr1->data = ptr1->next->data;
                ptr1->next->data = temp;
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }

    pthread_mutex_unlock(&l->mutex);
}

void *sort_thread(void *arg) {
    list *l = (list *)arg;
    while (!should_exit) {
        sleep(SORT_INTERVAL);
        bubble_sort(l);
    }
    return NULL;
}

int main(void) {
    list_init(&string_list);
    
    pthread_t sorter;
    if (pthread_create(&sorter, NULL, sort_thread, &string_list) != 0) {
        fprintf(stderr, "Failed to create sorting thread\n");
        list_destroy(&string_list);
        return EXIT_FAILURE;
    }

    char buffer[MAX_STRING_LENGTH + 1];
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        if (len == 0) {
            list_print(&string_list);
            continue;
        }

        for (size_t i = 0; i < len; i += MAX_STRING_LENGTH) {
            char temp[MAX_STRING_LENGTH + 1];
            size_t chunk_len = (len - i < MAX_STRING_LENGTH) ? len - i : MAX_STRING_LENGTH;
            strncpy(temp, buffer + i, chunk_len);
            temp[chunk_len] = '\0';
            list_insert(&string_list, temp);
        }
    }

    should_exit = 1;
    pthread_join(sorter, NULL);
    list_destroy(&string_list);

    return EXIT_SUCCESS;
}