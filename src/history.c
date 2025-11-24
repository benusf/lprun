#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "history.h"

static char *history_path()
{
    static char path[512];
    const char *home = getenv("HOME");
    if (!home) home = "/tmp";

    snprintf(path, sizeof(path),
             "%s/.programs/bin/lprun/history.log", home);
    return path;
}

void history_add(const char *printer, const char *file)
{
    FILE *f = fopen(history_path(), "a");
    if (!f) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(f, "%04d-%02d-%02d %02d:%02d:%02d | Printer: %s | File: %s\n",
            t->tm_year+1900, t->tm_mon+1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            printer, file);

    fclose(f);
}

void history_show()
{
    FILE *f = fopen(history_path(), "r");
    if (!f) {
        printf("No history found.\n");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }

    fclose(f);
}
