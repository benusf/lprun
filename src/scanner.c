#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"

static int run_cmd(const char *cmd)
{
    int rc = system(cmd);
    if (rc != 0)
        fprintf(stderr, "Scanner command failed: %s\n", cmd);
    return rc;
}

int scanner_scan_pdf(const char *outfile)
{
    char cmd[1024];

    // Requires ImageMagick "convert"
    snprintf(cmd, sizeof(cmd),
             "scanimage --format=png | convert - \"%s\"", outfile);

    return run_cmd(cmd);
}

int scanner_scan_img(const char *outfile)
{
    char cmd[1024];

    snprintf(cmd, sizeof(cmd),
             "scanimage --format=png > \"%s\"", outfile);

    return run_cmd(cmd);
}
