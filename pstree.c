#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <pthread.h>
#include "pstree.h"
#include <errno.h>

#include <fcntl.h>
#include <sys/mman.h>

#define ERR_shm_open 1
#define ERR_ftruncate 2
#define ERR_mmap 3
#define ERR_mutex_init 4
#define ERR_cond_variable 5
#define ERR_shm_unlink 6
#define ERR_invalid_td_or_uninitiazed_mem 7
#define ERR_INVALID_DESCRIPTOR 8
#define ERR_munmap 9
#define ERR_too_big_data 10
#define ERR_tree_none_exist 11
#define ERR_key_does_not_exist 12
#define ERR_memory 13
#define ERR_duplicate_key 14
typedef struct node {
    long key;
    int left_offset;
    int right_offset;
    int data_size;
    char data[];
} node;

typedef struct SharedTreeHeader {
    pthread_mutex_t lock;
    pthread_cond_t condition;

    int root_offset;
    int node_amount;
    int max_data_size;
    int memory_size;

    int free_list_head_offset;
} SharedTreeHeader;

void* sharedmemory = NULL;

int pst_errorcode;

static node* offset_to_node(int offset) {
    if (offset == -1)
        return NULL;
    return (node*)((char*)sharedmemory + offset);
}
                 
int pst_create(char *treename, int memsize, int maxdatasize)
{
    int shrdmmr;

    if (shm_unlink(treename) < 0 && errno != ENOENT) {
        pst_errorcode = ERR_shm_unlink;
        return PST_ERROR;
    }
    shrdmmr=shm_open(treename, O_CREAT | O_RDWR, 0666);
    if(shrdmmr<0){
        pst_errorcode=ERR_shm_open;
        return PST_ERROR;
    }


    if (ftruncate(shrdmmr, memsize) == -1) {
        pst_errorcode=ERR_ftruncate;
        close(shrdmmr);
        return PST_ERROR;
    }
    sharedmemory = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, shrdmmr, 0);
    if (sharedmemory == MAP_FAILED) {
        pst_errorcode=ERR_mmap;
        close(shrdmmr);
        return PST_ERROR;
    }
    close(shrdmmr);

    SharedTreeHeader *header = (SharedTreeHeader *)sharedmemory;
    header->max_data_size = maxdatasize;
    header->memory_size = memsize;
    header->root_offset = -1;
    header->node_amount = 0;
    header->free_list_head_offset = sizeof(SharedTreeHeader);
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;
    if (pthread_mutexattr_init(&mutex_attr) != 0 ||
        pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED) != 0 ||
        pthread_mutex_init(&header->lock, &mutex_attr) != 0) {
        pst_errorcode=ERR_mutex_init;
        munmap(sharedmemory, memsize);
        return PST_ERROR;
    }
    if (pthread_condattr_init(&cond_attr) != 0 ||
        pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED) != 0 ||
        pthread_cond_init(&header->condition, &cond_attr) != 0) {
        pst_errorcode=ERR_cond_variable;
        pthread_mutex_destroy(&header->lock);
        munmap(sharedmemory, memsize);
        return PST_ERROR;
    }
    return (PST_SUCCESS);
}
                 
int pst_destroy(char *treename)
{
    if (shm_unlink(treename) < 0) {
        pst_errorcode=ERR_shm_unlink;
        return PST_ERROR;
    }
    return PST_SUCCESS;
}

int pst_open(char *treename)
{
    int td = 1;
    int shmm=shm_open(treename,O_RDWR, 0);
    if(shmm<0){
        pst_errorcode=ERR_shm_open;
        return PST_ERROR;
    }
    SharedTreeHeader* temp;
    temp = (SharedTreeHeader*) mmap(NULL, sizeof(SharedTreeHeader), PROT_READ, MAP_SHARED, shmm, 0);
    if (temp == MAP_FAILED) {
        pst_errorcode = ERR_mmap;
        close(shmm);
        return PST_ERROR;
    }

    size_t memsize = temp->memory_size;
    munmap(temp, sizeof(SharedTreeHeader));

    
    sharedmemory = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, shmm, 0);
    if (sharedmemory == MAP_FAILED) {
        pst_errorcode = ERR_mmap;
        close(shmm);
        return PST_ERROR;
    }

    close(shmm);
    return td;
}

int pst_close(int td)
{
    if (td != 1 || sharedmemory == NULL) {
        pst_errorcode = ERR_INVALID_DESCRIPTOR;
        return PST_ERROR;
    }

    SharedTreeHeader* header = (SharedTreeHeader*)sharedmemory;

    if (munmap(sharedmemory, header->memory_size) < 0) {
        pst_errorcode = ERR_munmap;
        return PST_ERROR;
    }
    sharedmemory = NULL;
    return PST_SUCCESS;
}

int pst_get_maxdatasize(int td)
{
    if (td != 1 || sharedmemory == NULL) {
        pst_errorcode=ERR_invalid_td_or_uninitiazed_mem;
        return PST_ERROR;
    }

    SharedTreeHeader* header = (SharedTreeHeader*)sharedmemory;
    return header->max_data_size;
}

int pst_get_nodecount(int td) {
    if (td != 1 || sharedmemory == NULL) {
        pst_errorcode=ERR_invalid_td_or_uninitiazed_mem;
        return PST_ERROR;
    }

    SharedTreeHeader* header = (SharedTreeHeader*)sharedmemory;
    return header->node_amount;
}


int pst_insert(int td, long key, char *buf, int size)
{
    if(td != 1 || sharedmemory == NULL) {
        pst_errorcode = ERR_INVALID_DESCRIPTOR;
        return PST_ERROR;
    }

    SharedTreeHeader* header = (SharedTreeHeader*)sharedmemory;

    if (size > header->max_data_size) {
        pst_errorcode = ERR_too_big_data;
        return PST_ERROR;
    }

    int total_size = sizeof(node) + header->max_data_size;
    if (header->free_list_head_offset + total_size > header->memory_size) {
        pst_errorcode = ERR_memory;
        return PST_ERROR;
    }

    node* newnode = (node*)((char*)sharedmemory + header->free_list_head_offset);
    int newnode_offset = header->free_list_head_offset;
    header->free_list_head_offset += total_size;

   
    newnode->key = key;
    newnode->left_offset = -1;
    newnode->right_offset = -1;
    newnode->data_size = size;
    memcpy(newnode->data, buf, size);


    if (header->root_offset == -1) {
        header->root_offset = newnode_offset;
        header->node_amount++;
        return PST_SUCCESS;
    }

    
    node* current = offset_to_node(header->root_offset);

    while (current != NULL) {
        if (key == current->key) {
            pst_errorcode = ERR_duplicate_key;
            return PST_ERROR;
        }
        else if (key < current->key) {
            if (current->left_offset == -1) {
                current->left_offset = newnode_offset;
                header->node_amount++;
                return PST_SUCCESS;
            }
            current = offset_to_node(current->left_offset);
        }
        else {
            if (current->right_offset == -1) {
                current->right_offset = newnode_offset;
                header->node_amount++;
                return PST_SUCCESS;
            }
            current = offset_to_node(current->right_offset);
        }
    }

    return PST_ERROR;
}


int pst_update(int td, long key, char *buf, int size)
{
    if (td != 1 || sharedmemory == NULL) {
        pst_errorcode = ERR_INVALID_DESCRIPTOR;
        return PST_ERROR;
    }

    SharedTreeHeader* header = (SharedTreeHeader*)sharedmemory;

    
    if (size > header->max_data_size) {
        pst_errorcode = ERR_too_big_data;
        return PST_ERROR;
    }

    
    if (header->root_offset == -1) {
        pst_errorcode = ERR_tree_none_exist;
        return PST_ERROR;
    }

    node* current = (node*)((char*)sharedmemory + header->root_offset);

    while (current != NULL) {
        if (key == current->key) {
            
            memcpy(current->data, buf, size);
            current->data_size = size;
            return PST_SUCCESS;
        }
        else if (key < current->key) {
            if (current->left_offset == -1)
                break;
            current = (node*)((char*)sharedmemory + current->left_offset);
        }
        else { 
            if (current->right_offset == -1)
                break;
            current = (node*)((char*)sharedmemory + current->right_offset);
        }
    }

    pst_errorcode = ERR_key_does_not_exist;
    return PST_ERROR;
}


int pst_delete(int td, long key)
{
    if (td != 1 || sharedmemory == NULL) {
        pst_errorcode = ERR_INVALID_DESCRIPTOR;
        return PST_ERROR;
    }

    SharedTreeHeader* header = (SharedTreeHeader*)sharedmemory;

    int* link = &header->root_offset;
    node* current = offset_to_node(header->root_offset);

    
    while (current != NULL) {
        if (key == current->key) {
            break;
        }
        else if (key < current->key) {
            link = &current->left_offset;
            current = offset_to_node(current->left_offset);
        }
        else {
            link = &current->right_offset;
            current = offset_to_node(current->right_offset);
        }
    }

    if (current == NULL) {
        pst_errorcode = ERR_key_does_not_exist;
        return PST_ERROR;
    }

    //No children
    if (current->left_offset == -1 && current->right_offset == -1) {
        *link = -1;
    }
    //Only right child
    else if (current->left_offset == -1) {
        *link = current->right_offset;
    }
    //Only left child
    else if (current->right_offset == -1) {
        *link = current->left_offset;
    }
    //Two children
    else {
        
        int* min_link = &current->right_offset;
        node* min_node = offset_to_node(current->right_offset);

        while (min_node->left_offset != -1) {
            min_link = &min_node->left_offset;
            min_node = offset_to_node(min_node->left_offset);
        }
        current->key = min_node->key;
        memcpy(current->data, min_node->data, min_node->data_size);
        current->data_size = min_node->data_size;

        
        if (min_node->right_offset != -1)
            *min_link = min_node->right_offset;
        else
            *min_link = -1;
        header->node_amount--;
        return PST_SUCCESS;
    }

    header->node_amount--;
    return PST_SUCCESS;
}

int pst_get(int td, long key, char *buf)
{
    if (td != 1 || sharedmemory == NULL) {
        pst_errorcode = ERR_INVALID_DESCRIPTOR;
        return PST_ERROR;
    }

    SharedTreeHeader* header = (SharedTreeHeader*)sharedmemory;

    if (header->root_offset == -1) {
        pst_errorcode = ERR_key_does_not_exist;
        return PST_ERROR;
    }

    node* current = (node*)((char*)sharedmemory + header->root_offset);

    while (current != NULL) {
        if (key == current->key) {
            memcpy(buf, current->data, current->data_size);
            return current->data_size;
        }
        else if (key < current->key) {
            if (current->left_offset == -1)
                break;
            current = (node*)((char*)sharedmemory + current->left_offset);
        }
        else {
            if (current->right_offset == -1)
                break;
            current = (node*)((char*)sharedmemory + current->right_offset);
        }
    }

    pst_errorcode = ERR_key_does_not_exist;
    return PST_ERROR;
}

static void findkeys_recursive(node* current, long key1, long key2, int N, long keys[], int* count) {
    if (current == NULL || *count >= N) return;

    node* left = offset_to_node(current->left_offset);
    node* right = offset_to_node(current->right_offset);

    
    if (left) findkeys_recursive(left, key1, key2, N, keys, count);

    if (*count < N && current->key >= key1 && current->key <= key2) {
        keys[*count] = current->key;
        (*count)++;
    }

    if (right) findkeys_recursive(right, key1, key2, N, keys, count);
}

int pst_findkeys(int td, long key1, long key2, int N, long keys[])
{
    if (td != 1 || sharedmemory == NULL || N <= 0) {
        pst_errorcode = ERR_INVALID_DESCRIPTOR;
        return PST_ERROR;
    }

    SharedTreeHeader* header = (SharedTreeHeader*)sharedmemory;

    if (header->root_offset == -1) {
        return 0;
    }

    int count = 0;
    node* root = offset_to_node(header->root_offset);

    findkeys_recursive(root, key1, key2, N, keys, &count);

    return count;
}

int pst_printerror()
{
    switch (pst_errorcode) {
        case ERR_shm_open:
            printf("Error at shm_open function");
            break;
        case ERR_ftruncate:
            printf("Error at ftruncate function");
            break;
        case ERR_mmap:
            printf("Error at mmap function");
            break;
        case ERR_mutex_init:
            printf("");
            break;
        case ERR_cond_variable:
            printf("");
            break;
        case ERR_shm_unlink:
            printf("Error at shm_unlink function");
            break;
        case ERR_invalid_td_or_uninitiazed_mem:
            printf("td is invalid or memory is not initiazed");
            break;
        case ERR_INVALID_DESCRIPTOR:
            printf("");
            break;
        case ERR_munmap:
            printf("Errot at unmapping");
            break;
        case ERR_too_big_data:
            printf("Data is too big");
            break;
        case ERR_tree_none_exist:
            printf("Tree is not initialized yet");
            break;
        case ERR_key_does_not_exist:
            printf("Key does not exist");
            break;
        case ERR_memory:
            printf("Not enough memory left");
            break;
        case ERR_duplicate_key:
            printf("Key already exists");
            break;
      }
    return (PST_SUCCESS);
}