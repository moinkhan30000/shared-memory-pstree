#include "pstree.h"
#include <stdio.h>

#define TREENAME "/tree1"

int main() {
    int td = pst_open(TREENAME);
    char buf[100];
    int keys[] = {200, 201, 202, 203};

    for (int i = 0; i < 4; ++i) {
        if (pst_get(td, keys[i], buf) >= 0) {
            printf("Key %d found, data = %s\n", keys[i], buf);
        } else {
            printf("Key %d not found (might have been deleted)\n", keys[i]);
        }
    }

    pst_close(td);
    return 0;
}
