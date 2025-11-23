#define _POSIX_C_SOURCE 200809L
#include "print_cups.h"
#include <cups/cups.h>
#include <stdio.h>
#include <stdlib.h>

/* Submit a file to CUPS; returns job id (>0) or <=0 on failure */
int cups_print_file(const char *printer_name, const char *filename) {
    if (!printer_name || !filename) return -1;
    int job_id = cupsPrintFile(printer_name, filename, "myprinter-job", 0, NULL);
    return job_id;
}
