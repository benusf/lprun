#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "progress.h"

static long g_total = 0;
static char g_label[128];

void progress_init(const char *label, long total_bytes) {
    g_total = total_bytes;
    strncpy(g_label, label, sizeof(g_label));
    g_label[sizeof(g_label)-1] = '\0';

    printf("%s\n", g_label);
    printf("[------------------------------] 0%%\r");
    fflush(stdout);
}

void progress_update(long sent) {
    if (g_total <= 0) return;

    float percent = (float)sent / (float)g_total;
    if (percent > 1.0) percent = 1.0;

    int bars = (int)(percent * 30);

    printf("[");
    for (int i = 0; i < 30; i++)
        printf(i < bars ? "█" : "-");

    printf("] %3d%%", (int)(percent * 100));
    printf("  (%ld KB / %ld KB)\r", sent/1024, g_total/1024);
    fflush(stdout);
}

void progress_finish(void) {
    printf("\nDone.\n");
}
