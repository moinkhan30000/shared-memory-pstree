#include "pstree.h"
#include <stdio.h>
#include <string.h>

#define TREENAME "/tree1"


int main() {
    int td = pst_open(TREENAME);
    char data[100];

    for (int i = 100; i < 110; ++i) {
        sprintf(data, "bulk%d", i);
        pst_insert(td, i, data, strlen(data) + 1);
        printf("Inserted key %d\n", i);
    }

    pst_close(td);
    return 0;
}