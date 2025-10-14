// udp_recv_2step.c
// UDP receiver for 100GbE test:
// - First 8 bytes of each packet: little-endian u64 sequence counter (h)
// - Sender splits even seq to server A and odd seq to server B -> expect +2 step on each server
// - Print once per ~1s: bitrate (Gb/s) and packets per second (pps)
// - Options: -b bind_ip  -p port  -n max_pkts  -r rcvbuf_bytes  -C cpu_core

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <sched.h>

static inline uint64_t le64dec(const void *p) {
    const unsigned char *b = (const unsigned char*)p;
    return (uint64_t)b[0]       |
          ((uint64_t)b[1] << 8) |
          ((uint64_t)b[2] << 16)|
          ((uint64_t)b[3] << 24)|
          ((uint64_t)b[4] << 32)|
          ((uint64_t)b[5] << 40)|
          ((uint64_t)b[6] << 48)|
          ((uint64_t)b[7] << 56);
}

static double now_sec(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void usage(const char *prog){
    fprintf(stderr,
        "Usage: %s [-b bind_ip] [-p port] [-n max_pkts] [-r rcvbuf_bytes] [-C cpu_core]\n"
        "  -b  Bind IP (default 0.0.0.0)\n"
        "  -p  UDP port (default 60000)\n"
        "  -n  Max packets to read (0=infinite)\n"
        "  -r  SO_RCVBUF bytes (0=no change)\n"
        "  -C  Pin process to CPU core (optional)\n",
        prog);
}

int main(int argc, char **argv){
    const char *bind_ip = "0.0.0.0";
    int port = 60000;
    uint64_t max_pkts = 0;
    int rcvbuf = 0;
    int pin_core = -1;

    int opt;
    while ((opt = getopt(argc, argv, "b:p:n:r:C:h")) != -1){
        switch(opt){
            case 'b': bind_ip = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'n': max_pkts = strtoull(optarg, NULL, 10); break;
            case 'r': rcvbuf = atoi(optarg); break;
            case 'C': pin_core = atoi(optarg); break;
            case 'h': default: usage(argv[0]); return 1;
        }
    }

    if (pin_core >= 0) {
        cpu_set_t set; CPU_ZERO(&set); CPU_SET(pin_core, &set);
        if (sched_setaffinity(0, sizeof(set), &set) != 0) {
            perror("sched_setaffinity");
        } else {
            fprintf(stderr, "Pinned to CPU %d\n", pin_core);
        }
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 2; }

    if (rcvbuf > 0) {
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) != 0) {
            perror("setsockopt(SO_RCVBUF)");
        }
    }
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, bind_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid bind IP: %s\n", bind_ip);
        return 3;
    }
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        perror("bind");
        return 4;
    }

    const size_t FRAME_MAX = 16384; // cover 9k jumbo
    uint8_t *buf = (uint8_t*)malloc(FRAME_MAX);
    if (!buf) { perror("malloc"); return 5; }

    uint64_t expected = 0;
    int have_expected = 0;
    uint64_t pkts_total = 0, lost_steps = 0, parity_warn = 0;

    // per-second stats
    double t0 = now_sec();
    uint64_t bytes_sec = 0, pkts_sec = 0;

    printf("Listening on %s:%d ...\n", bind_ip, port);
    fflush(stdout);

    while (max_pkts == 0 || pkts_total < max_pkts) {
        ssize_t n = recvfrom(fd, buf, FRAME_MAX, 0, NULL, NULL);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom");
            break;
        }
        if (n < 8) continue; // need at least the 8-byte seq

        uint64_t seq = le64dec(buf);

        if (!have_expected) {
            expected = seq + 2;  // establish parity stream
            have_expected = 1;
        } else if (seq != expected) {
            if (seq > expected) {
                uint64_t delta = seq - expected;
                if ((delta & 1ULL) != 0ULL) {
                    // wrong parity: this server should not receive this seq
                    parity_warn++;
                    expected = seq + 2; // resync
                } else {
                    uint64_t missing = delta / 2ULL;
                    lost_steps += missing;
                    expected = seq + 2;
                }
            } else {
                // out-of-order/duplicate; ignore without moving expected
            }
        } else {
            expected += 2;
        }

        pkts_total++;
        pkts_sec++;
        bytes_sec += (uint64_t)n;

        // print every ~1 second
        double t1 = now_sec();
        if (t1 - t0 >= 1.0) {
            double dt = t1 - t0;
            double bps = (double)bytes_sec * 8.0 / dt;        // bits per second
            double gbps = bps / 1e9;
            double pps  = (double)pkts_sec / dt;

            printf("[server_2_1s] rate = %.3f Gb/s, pps = %.0f, lost(before) = %llu \n ",
                   gbps, pps,
                   (unsigned long long)lost_steps);
                  
            fflush(stdout);

            // reset 1-second counters
            t0 = t1;
            bytes_sec = 0;
            pkts_sec = 0;
        }
    }

    printf("Done. total_pkts=%llu, lost(step2)=%llu, parity=%llu\n",
           (unsigned long long)pkts_total,
           (unsigned long long)lost_steps,
           (unsigned long long)parity_warn);

    free(buf);
    close(fd);
    return 0;
}

