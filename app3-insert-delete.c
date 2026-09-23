#include "pstree.h"
#include <stdio.h>
#include <string.h>

#define TREENAME "/tree1"

int main() {
    int td = pst_open(TREENAME);

    char data[100];

    // Insert keys
    sprintf(data, "value200");
    pst_insert(td, 200, data, strlen(data) + 1);
    
    sprintf(data, "value201");
    pst_insert(td, 201, data, strlen(data) + 1);
    
    sprintf(data, "value202");
    pst_insert(td, 202, data, strlen(data) + 1);

    // Delete key
    pst_delete(td, 201);

    // Insert another
    sprintf(data, "value203");
    pst_insert(td, 203, data, strlen(data) + 1);

    pst_close(td);
    return 0;
}
