/* altitude_receiver.c
 * Usage: ./altitude_receiver [port] [csv_file]
 * Example: ./altitude_receiver 5000 received.csv
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>

#define DEFAULT_PORT 5000
#define LISTEN_BACKLOG 1
#define RECV_BUF 1024
#define LINE_BUF 8192

static volatile int keep_running = 1;
static void int_handler(int x) { (void)x; keep_running = 0; }

static int start_server(int port) {
    int listenfd;
    int opt = 1;
    struct sockaddr_in servaddr;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(listenfd);
        return -1;
    }
    if (listen(listenfd, LISTEN_BACKLOG) < 0) {
        perror("listen");
        close(listenfd);
        return -1;
    }
    return listenfd;
}

int main(int argc, char **argv) {
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;
    const char *csvfile = (argc > 2) ? argv[2] : "-";

    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);

    int listenfd = start_server(port);
    if (listenfd < 0) {
        fprintf(stderr, "Failed to start server on port %d\n", port);
        return 1;
    }
    printf("Receiver listening on port %d\n", port);
    printf("Waiting for client connection...\n");

    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
    if (connfd < 0) {
        perror("accept");
        close(listenfd);
        return 1;
    }
    char cli_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &cliaddr.sin_addr, cli_ip, sizeof(cli_ip));
    printf("Client connected from %s:%d\n", cli_ip, ntohs(cliaddr.sin_port));

    FILE *csv = NULL;
    if (csvfile && strcmp(csvfile, "-") != 0) {
        csv = fopen(csvfile, "w");
        if (!csv) {
            perror("fopen csv");
        } else {
            fprintf(csv, "index,altitude\n");
            fflush(csv);
        }
    }

    char recvbuf[RECV_BUF];
    char linebuf[LINE_BUF];
    size_t linepos = 0;

    double prev_alt = NAN;
    size_t total = 0;
    size_t anomalies = 0;

    while (keep_running) {
        ssize_t n = recv(connfd, recvbuf, sizeof(recvbuf), 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            break;
        } else if (n == 0) {
            printf("Client disconnected (EOF)\n");
            break;
        }
        for (ssize_t i = 0; i < n; ++i) {
            char c = recvbuf[i];
            if (c == '\n' || linepos >= LINE_BUF - 1) {
                linebuf[linepos] = '\0';
                /* parse altitude: format "index,altitude" or just "altitude" */
                char *comma = strrchr(linebuf, ',');
                char *numstr = (comma) ? (comma + 1) : linebuf;
                errno = 0;
                char *endptr = NULL;
                double alt = strtod(numstr, &endptr);
                if (endptr == numstr || errno != 0) {
                    fprintf(stderr, "Warning: could not parse altitude from '%s'\n", linebuf);
                } else {
                    total++;
                    if (!isnan(prev_alt)) {
                        double diff = fabs(alt - prev_alt);
                        if (diff > 100.0) {
                            anomalies++;
                            printf("[ANOMALY] sample %zu: jump %.3f ft (prev=%.3f -> cur=%.3f)\n",
                                   total, diff, prev_alt, alt);
                        }
                    }
                    prev_alt = alt;
                    if (csv) {
                        fprintf(csv, "%zu,%.3f\n", total, alt);
                        fflush(csv);
                    }
                    printf("Received sample %3zu: %.3f ft\n", total, alt);
                }
                linepos = 0;
            } else {
                linebuf[linepos++] = c;
            }
        }
    }

    printf("\n=== Summary ===\n");
    printf("Total samples received: %zu\n", total);
    printf("Anomalies detected (>100 ft/sec): %zu\n", anomalies);

    if (csv) fclose(csv);
    close(connfd);
    close(listenfd);
    return 0;
}
