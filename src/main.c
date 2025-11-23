#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cups/cups.h>

#include "disc.h"
#include "print_cups.h"
#include "print_raw.h"
#include "utils.h"
#include "printer_list.h"

// static void usage(const char *prog) {
//     fprintf(stderr,
//         "Usage: %s [--printer CUPS_NAME] [--ip IP] [--port PORT]\n"
//         "          (--text \"text\" | --image file | --file file) [--copies N]\n\n"
//         "Examples:\n"
//         "  %s --text \"Hello world\"\n"
//         "  %s --printer \"Canon_G3020_series\" --file doc.pdf\n"
//         "  %s --ip 192.168.1.40 --image photo.png --copies 2\n",
//         prog, prog, prog, prog);
// }


void print_usage(void) {
    printf("\n");
    printf("lprun - Command-line printer tool\n");
    printf("\nUsage:\n");
    printf("  lprun --list\n");
    printf("      List all available printers.\n");
    printf("\n");
    printf("  lprun --printer <name> --text \"Hello World\" [--copies N]\n");
    printf("      Print text to the specified printer.\n");
    printf("\n");
    printf("  lprun --printer <name> --image <file.png> [--copies N]\n");
    printf("      Print an image file to the specified printer.\n");
    printf("\nExamples:\n");
    printf("  lprun --list\n");
    printf("  lprun --printer Canon_G3020 --text \"Hello World\" --copies 2\n");
    printf("  lprun --printer Canon_G3020 --image img.png\n");
    printf("\n");
}

int main(int argc, char **argv) {
    const char *printer_name = NULL;
    const char *ip = NULL;
    int port = 9100;
    const char *text = NULL;
    const char *image = NULL;
    const char *file = NULL;
    int copies = 1;

    if (argc < 2) { print_usage(); return 1; }

    if (argc > 1) {
        if (strcmp(argv[1], "--list") == 0) {
            list_printers();
            return 0;  // exit after listing printers
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--printer") == 0 && i+1 < argc) printer_name = argv[++i];
        else if (strcmp(argv[i], "--ip") == 0 && i+1 < argc) ip = argv[++i];
        else if (strcmp(argv[i], "--port") == 0 && i+1 < argc) port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--text") == 0 && i+1 < argc) text = argv[++i];
        else if (strcmp(argv[i], "--image") == 0 && i+1 < argc) image = argv[++i];
        else if (strcmp(argv[i], "--file") == 0 && i+1 < argc) file = argv[++i];
        else if (strcmp(argv[i], "--copies") == 0 && i+1 < argc) copies = atoi(argv[++i]);
        else { print_usage(); return 1; }
    }

    printf("Usage: lprun --list\n");
    // TODO: handle printing, scanning, etc.

    if (!(text || image || file)) {
        fprintf(stderr, "Error: one of --text / --image / --file required\n");
        print_usage();
        return 2;
    }

    /* If printer name not provided, try to find one via CUPS or network discovery */
    if (!printer_name && !ip) {
        printf("Discovering CUPS/network printers...\n");
        /* prefer CUPS printers */
        char *cups_printer = discover_cups_printer();
        if (cups_printer) {
            printf("Found CUPS printer: %s\n", cups_printer);
            printer_name = cups_printer; /* ownership transferred to print path if used */
        } else {
            /* discover IP via avahi/nmap */
            char *found_ip = discover_printer_ip();
            if (found_ip) {
                printf("Found printer IP: %s\n", found_ip);
                ip = found_ip; /* will be freed later */
            } else {
                fprintf(stderr, "No printer discovered. Use --printer or --ip.\n");
                return 3;
            }
        }
    }

    /* Prepare an output file to send */
    char *out = NULL;
    int owns_out = 0;

    if (text) {
        out = create_temp_ps_from_text(text);
        if (!out) { fprintf(stderr, "Failed to create PS from text\n"); return 4; }
        owns_out = 1;
    } else if (image) {
        out = convert_image_to_ps(image);
        if (!out) { fprintf(stderr, "Failed to convert image\n"); return 5; }
        owns_out = 1;
    } else {
        /* file: if PDF => convert to PS for max compatibility; else send as-is */
        if (ends_with_ci(file, ".pdf")) {
            out = convert_pdf_to_ps(file);
            if (!out) { fprintf(stderr, "Failed to convert PDF\n"); return 6; }
            owns_out = 1;
        } else {
            out = strdup(file);
            owns_out = 0;
        }
    }

    int rc = 0;
    if (printer_name) {
        /* Use CUPS */
        printf("Sending to CUPS printer: %s (copies=%d)\n", printer_name, copies);
        for (int i = 0; i < copies; ++i) {
            int job = cups_print_file(printer_name, out);
            if (job <= 0) { fprintf(stderr, "CUPS print failed: %s\n", cupsLastErrorString()); rc = 20; break; }
            printf("Submitted job id: %d\n", job);
        }
    } else { /* ip path */
        printf("Sending to raw printer %s:%d (copies=%d)\n", ip, port, copies);
        rc = send_file_raw(ip, port, out, copies);
    }

    if (owns_out) {
        unlink(out);
        free(out);
    }
    if (printer_name && !strstr(printer_name, "CUPS-FOUND-") ) {
        /* If discover_cups_printer returned malloc'd string, free it. Reject if user passed literal */
        ; /* discover_cups_printer returns NULL or caller-owned pointer; ensure free policy in discover.c */
    }
    /* If discover_printer_ip allocated ip, it will be freed by utils on exit? we used ip pointer directly; free if allocated in discover.c */
    /* For simplicity free ip if allocated by discover_printer_ip (it returns malloc'd char*) */
    /* discover_printer_ip returns malloc'd string in our implementation */
    if (ip && strchr(ip, '.')) free((void*)ip);

    return rc;
}
