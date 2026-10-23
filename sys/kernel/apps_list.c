#include "kernel.h"

/*
* add modules, because kernel already 4k lines
*/

void apps_list() {
    print("                 o-------------------------o\n");
    print("                 |                         |\n");
    print("                 |        1.Filestats      |\n");
    print("                 |        2.base converter |\n");
    print("                 |                         |\n");
    print("                 |                         |\n");
    print("                 |                         |\n");
    print("                 |                         |\n");
    print("                 |       0.exit            |\n");
    print("                 o-------------------------o\n");
    char choose[5];
    read_line(choose, 5);
    if (strcmp(choose, "1") == 0) { clear(); app_file_stats(); }
    else if (strcmp(choose, "2") == 0) { clear(); app_base_converter(); }
    else if (strcmp(choose, "0") == 0) {}
}