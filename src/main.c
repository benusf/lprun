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
#include "history.h"
#include "scanner.h"

void print_usage(void) {
    printf("\n");
    printf("lprun - Advanced command-line printer & scanner tool\n");
    printf("----------------------------------------------------\n\n");

    printf("USAGE:\n");
    printf("  lprun --list\n");
    printf("  lprun --printer <name> [OPTIONS]\n");
    printf("  lprun --ip <address> [--port <port>] [OPTIONS]\n");
    printf("  lprun scanner [--pdf | --img]\n");
    printf("  lprun history\n");
    printf("\n");

    printf("PRINT OPTIONS:\n");
    printf("  --text \"STRING\"         Print plain text\n");
    printf("  --image <file>           Print an image (PNG/JPG/WebP)\n");
    printf("  --file <file>            Print any file (PDF, PS, etc.)\n");
    printf("  --copies N               Print multiple copies (default: 1)\n");
    printf("\n");

    printf("COLOR OPTIONS:\n");
    printf("  --color                  Force color printing\n");
    printf("  --grayscale              Convert to grayscale before printing\n");
    printf("                           (applies to text/image/pdf conversion)\n");
    printf("\n");

    printf("PRINTER SELECTION:\n");
    printf("  --list                   List available printers via CUPS\n");
    printf("  --printer <name>         Use a specific CUPS printer\n");
    printf("  --ip <addr>              Send raw job directly to printer (LAN)\n");
    printf("  --port <port>            Raw printing port (default: 9100)\n");
    printf("\n");

    printf("SCANNER MODULE:\n");
    printf("  lprun scanner --pdf      Scan a page and save as PDF\n");
    printf("  lprun scanner --img      Scan a page and save as PNG\n");
    printf("\n");

    printf("HISTORY:\n");
    printf("  lprun history            Show print history (coming soon)\n");
    printf("\n");

    printf("EXAMPLES:\n");
    printf("  lprun --list\n");
    printf("      List CUPS printers\n\n");

    printf("  lprun --printer Canon_G3020 --text \"Hello World\"\n");
    printf("      Print text using a CUPS-managed printer\n\n");

    printf("  lprun --printer Canon_G3020 --image img.png --copies 2\n");
    printf("      Print an image twice\n\n");

    printf("  lprun --file doc.pdf --grayscale\n");
    printf("      Convert PDF to grayscale before printing\n\n");

    printf("  lprun --ip 192.168.1.40 --file doc.pdf\n");
    printf("      Use RAW socket printing\n\n");

    printf("  lprun scanner --pdf\n");
    printf("      Scan a page as PDF\n\n");

    printf("----------------------------------------------------\n");
    printf("Visit: https://github.com/benusf/lprun\n");
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
    int color_mode = 0;

    if (argc < 2) { printf("For help use --help"); return 1; }

    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            print_usage();
            return 0;  // exit after listing printers
        }
    }

    if (argc > 1) {
        if (strcmp(argv[1], "--list") == 0) {
            list_printers();
            return 0;  // exit after listing printers
        }
    }

    // --- Scanner Commands ---
    if (strcmp(argv[1], "scanner") == 0) {
        if (argc < 4) {
            printf("Usage:\n");
            printf("  lprun scanner --pdf <output.pdf>\n");
            printf("  lprun scanner --img <output.png>\n");
            return 1;
        }

        const char *mode = argv[2];
        const char *outfile = argv[3];

        if (strcmp(mode, "--pdf") == 0) {
            return scanner_scan_pdf(outfile);
        }
        else if (strcmp(mode, "--img") == 0) {
            return scanner_scan_img(outfile);
        }
        else {
            fprintf(stderr, "Unknown scanner mode: %s\n", mode);
            return 1;
        }
    }

    if (color_mode == 1 && color_mode == 2) {
        fprintf(stderr, "Error: Cannot use both --color and --grayscale.\n");
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--printer") == 0 && i+1 < argc) printer_name = argv[++i];
        else if (strcmp(argv[i], "--ip") == 0 && i+1 < argc) ip = argv[++i];
        else if (strcmp(argv[i], "--port") == 0 && i+1 < argc) port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--text") == 0 && i+1 < argc) text = argv[++i];
        else if (strcmp(argv[i], "--image") == 0 && i+1 < argc) image = argv[++i];
        else if (strcmp(argv[i], "--file") == 0 && i+1 < argc) file = argv[++i];
        else if (strcmp(argv[i], "--copies") == 0 && i+1 < argc) copies = atoi(argv[++i]);
        else if (strcmp(argv[1], "history") == 0) { history_show(); return 0; }
        else if (strcmp(argv[i], "--color") == 0) color_mode = 1;
        else if (strcmp(argv[i], "--grayscale") == 0) color_mode = 2;

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
        out = create_temp_ps_from_text(text, color_mode);
        if (!out) { fprintf(stderr, "Failed to create PS from text\n"); return 4; }
        owns_out = 1;
    } else if (image) {
        out = convert_image_to_ps(image, color_mode);
        if (!out) { fprintf(stderr, "Failed to convert image\n"); return 5; }
        owns_out = 1;
    } else {
        /* file: if PDF => convert to PS for max compatibility; else send as-is */
        if (ends_with_ci(file, ".pdf")) {
            out = convert_pdf_to_ps(file, color_mode);
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
            cups_option_t *options = NULL;
            int num_options = 0;

            // progress bar
            int percent = (int)((100.0 * i) / copies);
            progress_bar(percent);

            if (color_mode == 1)
            num_options = cupsAddOption("ColorModel", "Color", num_options, &options);
            else if (color_mode == 2)
            num_options = cupsAddOption("ColorModel", "Gray", num_options, &options);

            int job = cupsPrintFile(printer_name, out, "lprun-job", num_options, options);

            cupsFreeOptions(num_options, options);
            if (job <= 0) { fprintf(stderr, "CUPS print failed: %s\n", cupsLastErrorString()); rc = 20; break; }
            printf("Submitted job id: %d\n", job);
            history_add(printer_name, file);
        }

        progress_bar(100);

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
