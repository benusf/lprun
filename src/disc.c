#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "disc.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* discover_cups_printer:
 *  - runs `lpinfo -v` and looks for first network or usb printer with a name
 *  - returns malloc'd C string with the CUPS printer name if found, else NULL
 */
char *discover_cups_printer(void) {
    FILE *fp = popen("lpstat -v 2>/dev/null", "r");
    if (!fp) return NULL;
    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        /* sample: device for Canon_G3020_series: usb://Canon/G3020%20series?serial=... */
        if (strstr(line, "device for ")) {
            char *p = strstr(line, "device for ");
            p += strlen("device for ");
            char *colon = strstr(p, ":");
            if (colon) {
                *colon = '\0';
                char *name = strdup(p);
                /* strip newline and spaces */
                trim(name);
                pclose(fp);
                return name;
            }
        }
    }
    pclose(fp);
    return NULL;
}

/* discover_printer_ip:
 *  Try avahi-browse first, then nmap scan of local /24 subnet for common printer ports
 *  returns malloc'd ip string or NULL
 */
char *discover_printer_ip(void) {
    /* Try avahi-browse */
    FILE *fp = popen("avahi-browse -rt _ipp._tcp --resolve --parsable 2>/dev/null", "r");
    if (fp) {
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            /* parsable format: ;enp3s0;IPv4;My Printer;_ipp._tcp;local;hostname.local;192.168.1.40;631; */
            char *cpy = strdup(line);
            char *save = cpy;
            char *tok;
            int field = 0;
            char *ip = NULL;
            while ((tok = strsep(&cpy, ";")) != NULL) {
                field++;
                if (field == 8 && tok && strlen(tok) > 0) {
                    ip = strdup(tok);
                    break;
                }
            }
            free(save);
            if (ip) { pclose(fp); return ip; }
        }
        pclose(fp);
    }

    /* Fallback: nmap scan on local subnet (requires nmap) */
    char *subnet = get_local_subnet_cidr();
    if (!subnet) return NULL;

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "nmap -p 9100,8611,631 --open -oG - %s 2>/dev/null", subnet);
    FILE *nfp = popen(cmd, "r");
    if (!nfp) { free(subnet); return NULL; }
    char line[1024];
    while (fgets(line, sizeof(line), nfp)) {
        if (strncmp(line, "Host:", 5) == 0) {
            /* Host: 192.168.1.40 ()    Ports: 9100/open/tcp//jetdirect/// */
            char *p = line + strlen("Host:");
            while (*p == ' ') p++;
            char ipbuf[64] = {0};
            sscanf(p, "%63s", ipbuf);
            if (strstr(line, "Ports:") && (strstr(line, "9100/open") || strstr(line, "8611/open") || strstr(line, "631/open"))) {
                char *res = strdup(ipbuf);
                pclose(nfp);
                free(subnet);
                return res;
            }
        }
    }
    pclose(nfp);
    free(subnet);
    return NULL;
}
