/* altitude_streamer.c
 * Usage: ./altitude_streamer [host] [port] [duration_seconds] [csv_file] [inject_second]
 * Example: ./altitude_streamer 127.0.0.1 5000 60 out.csv 30
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

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 5000
#define DEFAULT_DURATION 60
#define BUFFER_SZ 128

static int connect_to_server(const char *host, int port) {
    int sock;
    struct sockaddr_in servaddr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", host);
        close(sock);
        return -1;
    }
    if (connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }
    return sock;
}

static int send_all(int sock, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return -1;
        }
        sent += n;
    }
    return 0;
}

int main(int argc, char **argv) {
    const char *host = (argc > 1) ? argv[1] : DEFAULT_HOST;
    int port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;
    int duration = (argc > 3) ? atoi(argv[3]) : DEFAULT_DURATION;
    const char *csvfile = (argc > 4) ? argv[4] : "-";
    int inject_second = (argc > 5) ? atoi(argv[5]) : -1;

    if (duration <= 0) duration = DEFAULT_DURATION;
    if (port <= 0) port = DEFAULT_PORT;

    double *samples = calloc(duration, sizeof(double));
    if (!samples) {
        perror("calloc");
        return 1;
    }

    int sock = connect_to_server(host, port);
    if (sock < 0) {
        fprintf(stderr, "Could not connect to %s:%d — exiting\n", host, port);
        free(samples);
        return 1;
    }
    printf("Connected to %s:%d\n", host, port);

    FILE *csv = NULL;
    if (csvfile && strcmp(csvfile, "-") != 0) {
        csv = fopen(csvfile, "w");
        if (!csv) {
            perror("fopen csv");
            // continue without CSV saving
        } else {
            fprintf(csv, "index,altitude\n");
            fflush(csv);
        }
    }

    srand((unsigned)time(NULL));
    double altitude = 1000.0;         /* start altitude in ft */
    const double steady_inc = 2.0;    /* smooth climb per second (ft/sec) */

    for (int i = 0; i < duration; ++i) {
        /* If user requested to inject a spike for testing, do it */
        if (inject_second >= 0 && i == inject_second) {
            altitude += 150.0; /* a big unrealistic jump */
        } else {
            double turbulence = ((double)rand() / RAND_MAX) * 20.0 - 10.0; /* [-10,10] */
            altitude += steady_inc + turbulence;
        }
        samples[i] = altitude;

        char out[BUFFER_SZ];
        int written = snprintf(out, sizeof(out), "%d,%.3f\n", i + 1, altitude);
        if (written < 0) {
            fprintf(stderr, "Encoding error\n");
            break;
        }

        if (send_all(sock, out, (size_t)written) < 0) {
            perror("send");
            break;
        }

        if (csv) {
            fprintf(csv, "%d,%.3f\n", i + 1, altitude);
            fflush(csv);
        }

        printf("Sent sample %2d: %.3f ft\n", i + 1, altitude);
        if (i < duration - 1) sleep(1);
    }

    printf("Done sending. Closing socket.\n");
    close(sock);
    if (csv) fclose(csv);
    free(samples);
    return 0;
}
