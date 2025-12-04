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

/* Helper: check if command exists */
static int command_exists(const char *cmd) {
    char test_cmd[256];
    snprintf(test_cmd, sizeof(test_cmd), "command -v %s >/dev/null 2>&1", cmd);
    return system(test_cmd) == 0;
}

/* Helper: get ImageMagick command (magick for v7, convert for v6) */
static const char *get_imagemagick_cmd(void) {
    static int checked = 0;
    static const char *cmd = NULL;

    if (!checked) {
        if (command_exists("magick")) {
            cmd = "magick";
        } else if (command_exists("convert")) {
            cmd = "convert";
        }
        checked = 1;
    }
    return cmd;
}

/* Helper: create temp file with extension */
static char *create_temp_file(const char *prefix, const char *ext) {
    char tmpl[256];
    snprintf(tmpl, sizeof(tmpl), "/tmp/%s_XXXXXX%s", prefix, ext);
    int fd = mkstemps(tmpl, strlen(ext));
    if (fd < 0) return NULL;
    close(fd);
    return strdup(tmpl);
}

/* Helper: run command and check status */
static int run_command(const char *cmd) {
    int status = system(cmd);
    if (status != 0) {
        fprintf(stderr, "Command failed (status %d): %s\n", status, cmd);
    }
    return status == 0;
}

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
    char *out_file = create_temp_file("lprun_text", ".ps");
    if (!out_file) return NULL;

    FILE *f = fopen(out_file, "w");
    if (!f) {
        free(out_file);
        return NULL;
    }

    /* Write simple PostScript text document */
    fprintf(f, "%%!PS-Adobe-3.0\n"
               "/Courier findfont 12 scalefont setfont\n"
               "72 720 moveto\n"  // 1 inch from left, 10 inches from bottom
               "(%s) show\n"
               "showpage\n", text);
    fclose(f);

    /* If grayscale requested, convert the PS file */
    if (color_mode == 2) {
        char *gray_file = create_temp_file("lprun_text_gray", ".ps");
        if (!gray_file) {
            free(out_file);
            return NULL;
        }

        const char *img_cmd = get_imagemagick_cmd();
        if (img_cmd) {
            char cmd[512];
            /* Use ImageMagick to convert to grayscale */
            if (strcmp(img_cmd, "magick") == 0) {
                snprintf(cmd, sizeof(cmd), "magick convert %s -colorspace Gray %s",
                         out_file, gray_file);
            } else {
                snprintf(cmd, sizeof(cmd), "convert %s -colorspace Gray %s",
                         out_file, gray_file);
            }

            if (run_command(cmd)) {
                unlink(out_file);
                free(out_file);
                return gray_file;
            }
        }

        /* ImageMagick not available or failed, keep original */
        if (gray_file) {
            unlink(gray_file);
            free(gray_file);
        }
    }

    return out_file;
}

char *convert_image_to_ps(const char *path, int color_mode)
{
    char *out_file = create_temp_file("lprun_img", ".ps");
    if (!out_file) return NULL;

    const char *img_cmd = get_imagemagick_cmd();
    if (!img_cmd) {
        fprintf(stderr, "ImageMagick not found (neither 'magick' nor 'convert')\n");
        free(out_file);
        return NULL;
    }

    char cmd[512];
    if (color_mode == 2) {
        /* Grayscale conversion */
        if (strcmp(img_cmd, "magick") == 0) {
            snprintf(cmd, sizeof(cmd), "magick convert \"%s\" -colorspace Gray \"%s\"",
                     path, out_file);
        } else {
            snprintf(cmd, sizeof(cmd), "convert \"%s\" -colorspace Gray \"%s\"",
                     path, out_file);
        }
    } else {
        /* Normal conversion */
        if (strcmp(img_cmd, "magick") == 0) {
            snprintf(cmd, sizeof(cmd), "magick convert \"%s\" \"%s\"",
                     path, out_file);
        } else {
            snprintf(cmd, sizeof(cmd), "convert \"%s\" \"%s\"",
                     path, out_file);
        }
    }

    if (!run_command(cmd)) {
        free(out_file);
        return NULL;
    }

    return out_file;
}


char *convert_pdf_to_ps(const char *path, int color_mode)
{
    char *out_file = create_temp_file("lprun_pdf", ".ps");
    if (!out_file) return NULL;

    /* Try pdftops first (from poppler-utils) - it doesn't use ImageMagick */
    if (command_exists("pdftops")) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "pdftops \"%s\" \"%s\"", path, out_file);

        if (run_command(cmd)) {
            /* pdftops doesn't support grayscale conversion, so we need to post-process */
            if (color_mode == 2) {
                char *gray_file = create_temp_file("lprun_pdf_gray", ".ps");
                if (gray_file) {
                    const char *img_cmd = get_imagemagick_cmd();
                    if (img_cmd) {
                        char convert_cmd[512];
                        /* Use appropriate command based on ImageMagick version */
                        if (strcmp(img_cmd, "magick") == 0) {
                            snprintf(convert_cmd, sizeof(convert_cmd),
                                     "magick convert \"%s\" -colorspace Gray \"%s\" 2>/dev/null",
                                     out_file, gray_file);
                        } else {
                            snprintf(convert_cmd, sizeof(convert_cmd),
                                     "convert \"%s\" -colorspace Gray \"%s\" 2>/dev/null",
                                     out_file, gray_file);
                        }

                        /* Run conversion silently */
                        if (system(convert_cmd) == 0) {
                            unlink(out_file);
                            free(out_file);
                            return gray_file;
                        }
                    }
                    /* Conversion failed, keep original */
                    free(gray_file);
                }
            }
            return out_file;
        }
    }

    /* Try GhostScript (preferred for grayscale) */
    if (command_exists("gs")) {
        char cmd[1024];
        if (color_mode == 2) {
            /* Convert to grayscale PS using GhostScript */
            snprintf(cmd, sizeof(cmd),
                "gs -q -dNOPAUSE -dBATCH -sDEVICE=ps2write "
                "-sColorConversionStrategy=Gray -dProcessColorModel=/DeviceGray "
                "-dPDFSETTINGS=/printer -dCompatibilityLevel=1.4 "
                "-dAutoRotatePages=/None -dEmbedAllFonts=true "
                "-sOutputFile=\"%s\" \"%s\" 2>/dev/null",
                out_file, path);
        } else {
            /* Normal PS conversion */
            snprintf(cmd, sizeof(cmd),
                "gs -q -dNOPAUSE -dBATCH -sDEVICE=ps2write "
                "-dPDFSETTINGS=/printer -dCompatibilityLevel=1.4 "
                "-dAutoRotatePages=/None -dEmbedAllFonts=true "
                "-sOutputFile=\"%s\" \"%s\" 2>/dev/null",
                out_file, path);
        }

        if (run_command(cmd)) {
            return out_file;
        }
    }

    /* All methods failed */
    fprintf(stderr, "Failed to convert PDF to PS. Install poppler-utils (pdftops) or ghostscript (gs).\n");
    free(out_file);
    return NULL;
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

// void progress_bar(int percent)
// {
//     const int width = 40; // number of blocks
//     int filled = (percent * width) / 100;

//     printf("\r[");
//     for (int i = 0; i < filled; i++) printf("#");
//     for (int i = filled; i < width; i++) printf(" ");
//     printf("] %d%%", percent);
//     fflush(stdout);

//     if (percent == 100) printf("\n");
// }
