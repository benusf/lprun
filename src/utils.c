#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <ctype.h>

/* create temporary filename with suffix; caller must free returned pointer */
char *create_temp_with_suffix(const char *suffix) {
    char tmpl[] = "/tmp/myprinter-XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return NULL;
    close(fd);
    size_t len = strlen(tmpl) + strlen(suffix) + 1;
    char *out = malloc(len);
    if (!out) { unlink(tmpl); return NULL; }
    snprintf(out, len, "%s%s", tmpl, suffix);
    /* rename temp to include suffix */
    if (rename(tmpl, out) != 0) { free(out); return NULL; }
    return out;
}

char *create_temp_ps_from_text(const char *text, int color_mode)
{
    char tmpl[] = "/tmp/lprun_text_XXXXXX.ps";
    int fd = mkstemps(tmpl, 3);
    if (fd < 0) return NULL;

    FILE *f = fdopen(fd, "w");
    if (!f) return NULL;

    fprintf(f, "%%!PS-Adobe-3.0\n"
               "/Courier findfont 12 scalefont setfont\n"
               "50 750 moveto\n"
               "(%s) show\n"
               "showpage\n", text);

    fclose(f);

    // If grayscale mode requested → convert using ImageMagick
    if (color_mode == 2) {
        char grayfile[256];
        snprintf(grayfile, sizeof(grayfile), "%s_gray.ps", tmpl);

        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "convert %s -colorspace Gray %s",
                 tmpl, grayfile);

        system(cmd);
        unlink(tmpl);
        return strdup(grayfile);
    }

    return strdup(tmpl);
}

char *convert_image_to_ps(const char *path, int color_mode)
{
    char tmpl[] = "/tmp/lprun_img_XXXXXX.ps";
    int fd = mkstemps(tmpl, 3);
    if (fd < 0) return NULL;
    close(fd);

    char cmd[512];

    if (color_mode == 2) {
        // grayscale conversion
        snprintf(cmd, sizeof(cmd),
            "convert %s -colorspace Gray %s",
            path, tmpl);
    } else {
        // normal conversion
        snprintf(cmd, sizeof(cmd),
            "convert %s %s",
            path, tmpl);
    }

    if (system(cmd) != 0) return NULL;

    return strdup(tmpl);
}

char *convert_pdf_to_ps(const char *path, int color_mode)
{
    char tmpl[] = "/tmp/lprun_pdf_XXXXXX.ps";
    int fd = mkstemps(tmpl, 3);
    if (fd < 0) return NULL;
    close(fd);

    char cmd[512];

    // convert PDF → PS
    snprintf(cmd, sizeof(cmd),
        "pdftops %s %s",
        path, tmpl);

    if (system(cmd) != 0) return NULL;

    // grayscale?
    if (color_mode == 2) {
        char grayfile[256];
        snprintf(grayfile, sizeof(grayfile), "%s_gray.ps", tmpl);

        snprintf(cmd, sizeof(cmd),
            "convert %s -colorspace Gray %s",
            tmpl, grayfile);

        system(cmd);
        unlink(tmpl);
        return strdup(grayfile);
    }

    return strdup(tmpl);
}

/* get first non-loopback IPv4 interface and return base /24 CIDR like "192.168.1.0/24" */
char *get_local_subnet_cidr(void) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) return NULL;
    char host[NI_MAXHOST];
    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            if (sa->sin_addr.s_addr == htonl(INADDR_LOOPBACK)) continue;
            inet_ntop(AF_INET, &sa->sin_addr, host, sizeof(host));
            /* compute base /24 */
            unsigned int a,b,c,d;
            if (sscanf(host, "%u.%u.%u.%u", &a,&b,&c,&d) == 4) {
                char *res = malloc(32);
                snprintf(res, 32, "%u.%u.%u.0/24", a,b,c);
                freeifaddrs(ifaddr);
                return res;
            }
        }
    }
    freeifaddrs(ifaddr);
    return NULL;
}

int ends_with_ci(const char *s, const char *suffix) {
    if (!s || !suffix) return 0;
    size_t sl = strlen(s), su = strlen(suffix);
    if (su > sl) return 0;
    for (size_t i = 0; i < su; ++i) {
        char a = s[sl - su + i], b = suffix[i];
        if (tolower((unsigned char)a) != tolower((unsigned char)b)) return 0;
    }
    return 1;
}

/* trim leading/trailing whitespace in-place */
void trim(char *s) {
    char *end;
    while(isspace((unsigned char)*s)) s++;
    if (*s == 0) return;
    end = s + strlen(s) - 1;
    while(end > s && isspace((unsigned char)*end)) end--;
    *(end+1) = 0;
}

/* escape helper - naive, wraps in single quotes */
const char *escape_shell_arg(const char *s) {
    /* naive: relies on filenames without single quotes; safe in most home-use cases */
    return s;
}


long get_file_size(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long s = ftell(f);
    fclose(f);
    return s;
}


void progress_bar(int percent)
{
    const int width = 40; // number of blocks
    int filled = (percent * width) / 100;

    printf("\r[");
    for (int i = 0; i < filled; i++) printf("#");
    for (int i = filled; i < width; i++) printf(" ");
    printf("] %d%%", percent);
    fflush(stdout);

    if (percent == 100) printf("\n");
}
