#define _POSIX_C_SOURCE 200809L
#include "print_raw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>

/* send file to ip:port copies times, returns 0 on success */
int send_file_raw(const char *ip, int port, const char *filename, int copies) {
    if (!ip || !filename) return -1;
    for (int c = 0; c < copies; ++c) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) { perror("socket"); return -2; }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
            fprintf(stderr, "Invalid IP: %s\n", ip);
            close(sock);
            return -3;
        }

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("connect");
            close(sock);
            return -4;
        }

        FILE *f = fopen(filename, "rb");
        if (!f) { perror("fopen"); close(sock); return -5; }

        char buf[8192];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
            ssize_t sent = 0;
            while (sent < (ssize_t)n) {
                ssize_t s = send(sock, buf + sent, n - sent, 0);
                if (s < 0) { perror("send"); fclose(f); close(sock); return -6; }
                sent += s;
            }
        }

        fclose(f);
        close(sock);
        printf("Sent copy %d/%d\n", c+1, copies);

        /* short pause between copies */
        struct timespec ts = {0, 200000000}; /* 200ms */
        nanosleep(&ts, NULL);
    }
    return 0;
}
