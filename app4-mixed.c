#include "pstree.h"
#include <stdio.h>
#include <string.h>

#define TREENAME "/tree1"

int main() {
    int td = pst_open(TREENAME);
    char data[100], buf[100];


    sprintf(data, "data300");
    pst_insert(td, 300, data, strlen(data) + 1);
    printf("Inserted 300\n");


    if (pst_get(td, 300, buf) >= 0)
        printf("Found 300: %s\n", buf);


    sprintf(data, "data301");
    pst_insert(td, 301, data, strlen(data) + 1);
    sprintf(data, "data302");
    pst_insert(td, 302, data, strlen(data) + 1);


    pst_delete(td, 301);
    printf("Deleted 301\n");


    if (pst_get(td, 301, buf) >= 0)
        printf("Found 301: %s\n", buf);
    else
        printf("301 not found (deleted)\n");


    sprintf(data, "data303");
    pst_insert(td, 303, data, strlen(data) + 1);


    if (pst_get(td, 303, buf) >= 0)
        printf("Found 303: %s\n", buf);

    pst_close(td);
    pst_destroy(TREENAME);
    return 0;
}
