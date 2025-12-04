#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>  // ADD THIS LINE
#include "scanner.h"

/* Create directory if missing */
static int ensure_dir(const char *path)
{
    struct stat st = {0};

    if (stat(path, &st) == -1)
    {
        if (mkdir(path, 0755) != 0)
        {
            perror("mkdir");
            return -1;
        }
    }
    return 0;
}

/* Execute command and show error if fails */
static int run_cmd(const char *cmd)
{
    int rc = system(cmd);
    if (rc != 0)
        fprintf(stderr, "Scanner command failed: %s\n", cmd);
    return rc;
}

/* Generates a timestamp file path */
static void build_output_path(char *dest, size_t size,
                              const char *out_dir,
                              const char *ext)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    snprintf(dest, size,
             "%s/scan_%04d%02d%02d_%02d%02d%02d.%s",
             out_dir,
             tm->tm_year + 1900,
             tm->tm_mon + 1,
             tm->tm_mday,
             tm->tm_hour,
             tm->tm_min,
             tm->tm_sec,
             ext);
}

/* Atomic flag for stopping spinner */
static atomic_bool scanning_stop = false;

/* Spinner thread function */
static void* spinner_thread_func(void* arg) 
{
    (void)arg;  // Mark parameter as unused to avoid warning
    const char spin_chars[] = "|/-\\";
    int spin_index = 0;
    
    printf("Scanning... ");
    fflush(stdout);
    
    while (!atomic_load(&scanning_stop)) {
        printf("\b%c", spin_chars[spin_index]);
        fflush(stdout);
        spin_index = (spin_index + 1) % 4;
        usleep(150000); // 150ms
    }
    
    printf("\b \b\b"); // Clean up spinner
    return NULL;
}

int scanner_scan_pdf(const char *out_dir, const char *filename)
{
    /* Ensure the output directory exists */
    if (ensure_dir(out_dir) != 0)
    {
        fprintf(stderr, "Error: ✗ cannot create directory %s\n", out_dir);
        return 1;
    }

    char file[512];
    // Check if filename is absolute path
    if (filename[0] == '/') {
        strncpy(file, filename, sizeof(file));
        // Ensure directory exists
        char dir_part[512];
        strncpy(dir_part, filename, sizeof(dir_part));
        char *last_slash = strrchr(dir_part, '/');
        if (last_slash) {
            *last_slash = '\0';
            ensure_dir(dir_part);
        }
    } else {
        snprintf(file, sizeof(file), "%s/%s", out_dir, filename);
    }

    printf("Saving scan to: %s\n", file);

    // Start spinner thread
    pthread_t spinner_tid;
    atomic_store(&scanning_stop, false);  // Reset flag
    pthread_create(&spinner_tid, NULL, spinner_thread_func, NULL);
    
    // Run scan command
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "scanimage --format=pnm 2>/dev/null | convert - \"%s\" 2>/dev/null", file);
    
    int rc = system(cmd);
    
    // Stop spinner thread
    atomic_store(&scanning_stop, true);
    pthread_join(spinner_tid, NULL);
    
    if (rc == 0) {
        printf("\n");
        printf("✓ Scan completed successfully!\n");
    } else {
        printf("\n");
        fprintf(stderr, "✗ Scan failed with code %d\n", rc);
    }
    
    return rc;
}

int scanner_scan_img(const char *out_dir, const char *filename)
{
    /* Ensure the output directory exists */
    if (ensure_dir(out_dir) != 0)
    {
        fprintf(stderr, "Error: cannot create directory %s\n", out_dir);
        return 1;
    }

    char file[512];
    // Check if filename is absolute path
    if (filename[0] == '/') {
        strncpy(file, filename, sizeof(file));
        // Ensure directory exists
        char dir_part[512];
        strncpy(dir_part, filename, sizeof(dir_part));
        char *last_slash = strrchr(dir_part, '/');
        if (last_slash) {
            *last_slash = '\0';
            ensure_dir(dir_part);
        }
    } else {
        snprintf(file, sizeof(file), "%s/%s", out_dir, filename);
    }

    printf("Saving scan to: %s\n", file);

    // Start spinner thread
    pthread_t spinner_tid;
    atomic_store(&scanning_stop, false);  // Reset flag
    pthread_create(&spinner_tid, NULL, spinner_thread_func, NULL);
    
    // Run scan command
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "scanimage --format=png > \"%s\" 2>/dev/null", file);
    
    int rc = system(cmd);
    
    // Stop spinner thread
    atomic_store(&scanning_stop, true);
    pthread_join(spinner_tid, NULL);
    
    if (rc == 0) {
        printf("Scan completed successfully!\n");
    } else {
        fprintf(stderr, "Scan failed with code %d\n", rc);
    }
    
    return rc;
}
