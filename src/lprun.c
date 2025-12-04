#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <cups/cups.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#include "disc.h"
#include "print_cups.h"
#include "print_raw.h"
#include "utils.h"
#include "printer_list.h"
#include "history.h"
#include "scanner.h"

/* Global variables for progress indicators */
static atomic_bool printing_stop = false;


void animate_progress(const char *message, int duration_ms) {
    printf("%s ", message);
    fflush(stdout);

    const char spinner[] = "|/-\\";
    int spin_idx = 0;
    int steps = duration_ms / 150;

    for (int i = 0; i < steps; i++) {
        printf("\b%c", spinner[spin_idx]);
        fflush(stdout);
        spin_idx = (spin_idx + 1) % 4;
        usleep(150000); // 150ms
    }

    printf("\b \n"); // Clear spinner
}


/* Clear current line */
void clear_line(void) {
    printf("\r\033[K"); // \033[K clears from cursor to end of line
    fflush(stdout);
}

/* Print with line clear */
void print_clear(const char *format, ...) {
    va_list args;
    va_start(args, format);
    clear_line();
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

/* Spinner thread for printing */
void* printing_spinner_func(void* arg) {
    (void)arg; // Unused parameter
    const char spin_chars[] = "|/-\\";
    int spin_index = 0;

    printf("Printing ");
    fflush(stdout);

    while (!atomic_load(&printing_stop)) {
        printf("\b%c", spin_chars[spin_index]);
        fflush(stdout);
        spin_index = (spin_index + 1) % 4;
        usleep(150000); // 150ms
    }

    printf("\b \b\b"); // Clean up spinner
    return NULL;
}

void print_usage(void) {
    printf("\n");
    printf("lprun - Advanced command-line printer & scanner tool\n");
    printf("----------------------------------------------------\n\n");

    printf("USAGE:\n");
    printf("  lprun --list\n");
    printf("  lprun --printer <name> [OPTIONS]\n");
    printf("  lprun --ip <address> [--port <port>] [OPTIONS]\n");
    printf("  lprun scanner [--pdf | --img] <output_file>\n");
    printf("  lprun history\n");
    printf("\n");

    printf("PRINT OPTIONS:\n");
    printf("  --text \"STRING\"         Print plain text\n");
    printf("  --image <file>           Print an image (PNG/JPG/WebP)\n");
    printf("  --file <file>            Print any file (PDF, PS, etc.)\n");
    printf("  --copies N               Print multiple copies (default: 1)\n");
    printf("\n");

    printf("COLOR OPTIONS (mutually exclusive):\n");
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
    printf("  lprun scanner --pdf <output.pdf>\n");
    printf("  lprun scanner --img <output.png>\n");
    printf("\n");

    printf("HISTORY:\n");
    printf("  lprun history            Show print history\n");
    printf("\n");

    printf("EXAMPLES:\n");
    printf("  lprun --list\n");
    printf("      List CUPS printers\n\n");

    printf("  lprun --printer Canon_G3020 --text \"Hello World\"\n");
    printf("      Print text using a CUPS-managed printer\n\n");

    printf("  lprun --printer Canon_G3020 --image img.png --copies 2 --color\n");
    printf("      Print an image twice in color\n\n");

    printf("  lprun --file doc.pdf --grayscale\n");
    printf("      Convert PDF to grayscale before printing\n\n");

    printf("  lprun --ip 192.168.1.40 --file doc.pdf\n");
    printf("      Use RAW socket printing\n\n");

    printf("  lprun scanner --pdf scan.pdf\n");
    printf("      Scan a page as PDF\n\n");

    printf("----------------------------------------------------\n");
    printf("Visit: https://github.com/benusf/lprun\n");
    printf("\n");
}

int ensure_dir(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) {
            perror("mkdir");
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    const char *printer_name = NULL;
    const char *ip = NULL;
    int port = 9100;
    const char *text = NULL;
    const char *image = NULL;
    const char *file = NULL;
    int copies = 1;
    int color_mode = 0; // 0 = auto/default, 1 = color, 2 = grayscale
    char out_dir[512] = {0};

    if (argc < 2) {
        printf("For help use --help\n");
        return 1;
    }

    /* Check for --help first */
    if (strcmp(argv[1], "--help") == 0) {
        print_usage();
        return 0;
    }

    /* Check for --list */
    if (strcmp(argv[1], "--list") == 0) {
        list_printers();
        return 0;
    }

    /* Check for history */
    if (strcmp(argv[1], "history") == 0) {
        history_show();
        return 0;
    }

    /* --- Scanner Commands --- */
    if (strcmp(argv[1], "scanner") == 0) {
        if (argc < 4) {
            printf("Usage:\n");
            printf("  lprun scanner --pdf <output.pdf>\n");
            printf("  lprun scanner --img <output.png>\n");
            return 1;
        }

        const char *mode = argv[2];
        const char *filename = argv[3];

        if (out_dir[0] == '\0') {
            strcpy(out_dir, getenv("HOME"));
            strcat(out_dir, "/.local/share/lprun/scans");
        }

        ensure_dir(out_dir);

        if (strcmp(mode, "--pdf") == 0) {
            return scanner_scan_pdf(out_dir, filename);
        } else if (strcmp(mode, "--img") == 0) {
            return scanner_scan_img(out_dir, filename);
        } else {
            fprintf(stderr, "Unknown scanner mode: %s\n", mode);
            return 1;
        }
    }

    /* Parse command line arguments */
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--printer") == 0 && i+1 < argc) {
            printer_name = argv[++i];
        }
        else if (strcmp(argv[i], "--ip") == 0 && i+1 < argc) {
            ip = argv[++i];
        }
        else if (strcmp(argv[i], "--port") == 0 && i+1 < argc) {
            port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--text") == 0 && i+1 < argc) {
            text = argv[++i];
        }
        else if (strcmp(argv[i], "--image") == 0 && i+1 < argc) {
            image = argv[++i];
        }
        else if (strcmp(argv[i], "--file") == 0 && i+1 < argc) {
            file = argv[++i];
        }
        else if (strcmp(argv[i], "--copies") == 0 && i+1 < argc) {
            copies = atoi(argv[++i]);
            if (copies < 1) copies = 1;
        }
        else if (strcmp(argv[i], "--color") == 0) {
            if (color_mode == 2) {
                fprintf(stderr, "Error: Cannot use both --color and --grayscale\n");
                return 1;
            }
            color_mode = 1;
        }
        else if (strcmp(argv[i], "--grayscale") == 0) {
            if (color_mode == 1) {
                fprintf(stderr, "Error: Cannot use both --color and --grayscale\n");
                return 1;
            }
            color_mode = 2;
        }
        else if (strcmp(argv[i], "--out-dir") == 0 && i + 1 < argc) {
            strncpy(out_dir, argv[i+1], sizeof(out_dir)-1);
            i++;
        }
        else if (i == 1) {
            /* First argument wasn't handled above, show usage */
            print_usage();
            return 1;
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage();
            return 1;
        }
    }

    /* Validate we have something to print */
    if (!(text || image || file)) {
        fprintf(stderr, "Error: one of --text / --image / --file required\n");
        print_usage();
        return 2;
    }

    /* Validate color options */
    if (color_mode == 1 && color_mode == 2) {
        fprintf(stderr, "Error: Cannot use both --color and --grayscale\n");
        return 1;
    }

    /* If printer name not provided, try to find one via CUPS or network discovery */
    if (!printer_name && !ip) {
        printf("Discovering CUPS/network printers...\n");
        print_progress_bar(10);
        fflush(stdout);

        /* prefer CUPS printers */
        char *cups_printer = discover_cups_printer();
        clear_line();

        if (cups_printer) {
            printf("\rFound CUPS printer: %s\n", cups_printer);
            printer_name = cups_printer;
        } else {
            /* discover IP via avahi/nmap */
            char *found_ip = discover_printer_ip();

            if (found_ip) {
                printf("\rFound printer IP: %s\n", found_ip);
                ip = found_ip;
            } else {
                fprintf(stderr, "\nNo printer discovered. Use --printer or --ip.\n");
                return 3;
            }
        }
    }g

    /* Prepare an output file to send */
    char *out = NULL;
    int owns_out = 0;

    printf("Preparing document...\n");

    // Show animated progress while preparing
    pthread_t prep_spinner_tid;
    atomic_store(&printing_stop, false);
    pthread_create(&prep_spinner_tid, NULL, printing_spinner_func, NULL);

    if (text) {
        out = create_temp_ps_from_text(text, color_mode);
        if (!out) {
            atomic_store(&printing_stop, true);
            pthread_join(prep_spinner_tid, NULL);
            fprintf(stderr, "Failed to create PS from text\n");
            return 4;
        }
        owns_out = 1;
    } else if (image) {
        out = convert_image_to_ps(image, color_mode);
        if (!out) {
            atomic_store(&printing_stop, true);
            pthread_join(prep_spinner_tid, NULL);
            fprintf(stderr, "Failed to convert image\n");
            return 5;
        }
        owns_out = 1;
    } else {
        if (ends_with_ci(file, ".pdf")) {
            out = convert_pdf_to_ps(file, color_mode);
            if (!out) {
                atomic_store(&printing_stop, true);
                pthread_join(prep_spinner_tid, NULL);
                fprintf(stderr, "Failed to convert PDF\n");
                return 6;
            }
            owns_out = 1;
        } else {
            out = strdup(file);
            owns_out = 0;
        }
    }

    // Stop spinner and show success
    atomic_store(&printing_stop, true);
    pthread_join(prep_spinner_tid, NULL);
    printf("\rDocument prepared successfully.          \n");
    int rc = 0;
    if (printer_name) {
        /* Use CUPS */
        printf("Sending to CUPS printer: %s (copies=%d)\n", printer_name, copies);

        // Start printing spinner
        pthread_t print_spinner_tid;
        atomic_store(&printing_stop, false);
        pthread_create(&print_spinner_tid, NULL, printing_spinner_func, NULL);

        int successful_copies = 0;
        for (int i = 0; i < copies; ++i) {
            cups_option_t *options = NULL;
            int num_options = 0;

            // Show copy progress
            printf("\rCopy %d/%d ", i + 1, copies);
            fflush(stdout);

            // Set color mode if specified
            if (color_mode == 1) {
                num_options = cupsAddOption("ColorModel", "RGB", num_options, &options);
                num_options = cupsAddOption("ColorSpace", "sRGB", num_options, &options);
            } else if (color_mode == 2) {
                num_options = cupsAddOption("ColorModel", "Gray", num_options, &options);
                num_options = cupsAddOption("ColorSpace", "Gray", num_options, &options);
            }
            // For auto mode, don't set any color options - let printer decide

            int job = cupsPrintFile(printer_name, out, "lprun-job", num_options, options);

            cupsFreeOptions(num_options, options);
            if (job <= 0) {
                clear_line();
                fprintf(stderr, "CUPS print failed: %s\n", cupsLastErrorString());
                rc = 20;
                break;
            }

            successful_copies++;
            clear_line();
            printf("Copy %d/%d submitted (job id: %d)\n", successful_copies, copies, job);

            // Small delay between copies for better progress visibility
            if (i < copies - 1) {
                usleep(200000);
            }
        }

        // Stop spinner
        atomic_store(&printing_stop, true);
        pthread_join(print_spinner_tid, NULL);

        if (successful_copies > 0) {
            printf("✓ %d copy(ies) submitted successfully!\n", successful_copies);
        }

    } else {
        /* IP path - raw printing */
        printf("Sending to raw printer %s:%d (copies=%d)\n", ip, port, copies);

        // Start spinner for raw printing
        pthread_t raw_spinner_tid;
        atomic_store(&printing_stop, false);
        pthread_create(&raw_spinner_tid, NULL, printing_spinner_func, NULL);

        rc = send_file_raw(ip, port, out, copies);

        // Stop spinner
        atomic_store(&printing_stop, true);
        pthread_join(raw_spinner_tid, NULL);

        if (rc == 0) {
            clear_line();
            printf("✓ Raw print job sent successfully!\n");
        } else {
            clear_line();
            printf("✗ Raw print failed\n");
        }
    }

    /* Cleanup */
    if (owns_out) {
        unlink(out);
        free(out);
    }

    /* Free dynamically allocated printer name if it came from discovery */
    if (printer_name && printer_name != argv[2]) { // Not from command line
        free((void*)printer_name);
    }

    /* Free dynamically allocated IP if it came from discovery */
    if (ip && ip != argv[2]) { // Not from command line
        free((void*)ip);
    }

    return rc;
}
