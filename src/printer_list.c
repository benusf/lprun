#include <cups/cups.h>
#include <stdio.h>
#include "printer_list.h"

void list_printers(void) {
    cups_dest_t *dests;
    int num_dests = cupsGetDests(&dests);

    if(num_dests == 0) {
        printf("No printers found.\n");
        return;
    }

    printf("Available printers:\n");
    for(int i = 0; i < num_dests; i++) {
        printf("  %s", dests[i].name);
        if(dests[i].is_default)
            printf(" (default)");
        printf("\n");
    }

    cupsFreeDests(num_dests, dests);
}
