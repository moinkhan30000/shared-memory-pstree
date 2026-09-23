
#include "pstree.h"
#include <stdio.h>



#define TREENAME "/tree1"


int main() {
    
    int td = pst_open(TREENAME);
    if (td < 0) {
        printf("Error opening tree\n");
        return -1;
    }
    char buf[100];

    for (int i = 100; i < 110; ++i) {
        if (pst_get(td, i, buf) >= 0) {
            printf("Key %d found, data = %s\n", i, buf);
        } else {
            printf("Key %d not found\n", i);
        }
    }

    pst_close(td);
    return 0;
}
