#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUF_SIZE     1500
#define CHUNK_BYTES  20      // must match client
#define MAX_CHUNKS   10000   // adjust as needed

#pragma pack(push, 1)
struct DNS_ANSWER {
    unsigned short name;
    unsigned short type;
    unsigned short class;
    unsigned int   ttl;
    unsigned short data_len;
    unsigned int   address;
};
#pragma pack(pop)

typedef struct {
    int seen;
    int len;
    unsigned char data[CHUNK_BYTES];
} Chunk;

static Chunk chunks[MAX_CHUNKS];
static int have_start = 0;
static int total_bytes = -1;
static int expected_chunks = -1;
static int last_seq = 0;

// ---------- helpers ----------

int is_digits(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; s++) {
        if (*s < '0' || *s > '9') return 0;
    }
    return 1;
}

int is_hex_str(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; s++) {
        char c = *s;
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F')))
            return 0;
    }
    return 1;
}

// Fully correct DNS name decoder with pointer support
int decode_domain_name(const unsigned char *packet, int pkt_len,
                       int offset, unsigned char *out, int out_len)
{
    int out_i = 0;
    int jumped = 0;
    int orig_offset = offset;
    int loops = 0;

    while (offset < pkt_len && loops < 50) {
        loops++;

        unsigned char len = packet[offset];

        if (len == 0) {
            if (!jumped) offset++;
            break;
        }

        if ((len & 0xC0) == 0xC0) {
            if (offset + 1 >= pkt_len) return -1;
            int ptr = ((len & 0x3F) << 8) | packet[offset + 1];
            if (ptr >= pkt_len) return -1;
            offset = ptr;
            jumped = 1;
            continue;
        }

        offset++;
        if (offset + len > pkt_len) return -1;
        if (out_i + len + 1 >= out_len) return -1;

        memcpy(out + out_i, packet + offset, len);
        out_i += len;
        out[out_i++] = '.';

        offset += len;
    }

    if (out_i == 0) {
        out[0] = '\0';
    } else {
        out[out_i - 1] = '\0';
    }

    return jumped ? orig_offset + 2 : offset;
}

unsigned char hex_to_byte(const char *hex) {
    unsigned char val = 0;
    for (int i = 0; i < 2; i++) {
        char c = hex[i];
        val <<= 4;
        if (c >= '0' && c <= '9') val += c - '0';
        else if (c >= 'a' && c <= 'f') val += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') val += c - 'A' + 10;
    }
    return val;
}

void reset_state(void) {
    memset(chunks, 0, sizeof(chunks));
    have_start = 0;
    total_bytes = -1;
    expected_chunks = -1;
    last_seq = 0;
}

// ---------- main ----------

int main() {
    int sockfd;
    struct sockaddr_in server, client;
    unsigned char buffer[BUF_SIZE];
    socklen_t client_len = sizeof(client);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket failed"); return 1; }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(53);

    if (bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return 1;
    }

    reset_state();

    printf("Bind successful - DNS Tunnel Server is UP!\n");

    int done = 0;

    while (!done) {
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0,
                         (struct sockaddr*)&client, &client_len);
        if (n < 0) continue;

        unsigned char domain[512];
        int qname_off = 12;
        int new_off = decode_domain_name(buffer, n, qname_off,
                                         domain, sizeof(domain));
        if (new_off < 0) {
            printf("Failed to decode domain name\n");
            continue;
        }

        char dom_copy[512];
        strncpy(dom_copy, (char*)domain, sizeof(dom_copy) - 1);
        dom_copy[sizeof(dom_copy) - 1] = '\0';

        char *labels[10];
        int label_count = 0;
        char *tok = strtok(dom_copy, ".");
        while (tok && label_count < 10) {
            labels[label_count++] = tok;
            tok = strtok(NULL, ".");
        }

        if (label_count >= 3) {
            char type = labels[0][0];

            // ---------- START packet: s<total_bytes>.i0000.e0.suffix ----------
            if (type == 's') {
                const char *size_str = labels[0] + 1;
                if (!is_digits(size_str)) {
                    printf("Invalid START size: %s\n", size_str);
                } else if (labels[1][0] != 'i' || !is_digits(labels[1] + 1)) {
                    printf("Invalid START seq label: %s\n", labels[1]);
                } else if (strcmp(labels[1] + 1, "0000") != 0) {
                    printf("START seq must be 0000, got %s\n", labels[1] + 1);
                } else if (strcmp(labels[2], "e0") != 0) {
                    printf("START endflag must be e0, got %s\n", labels[2]);
                } else {
                    reset_state();
                    total_bytes = atoi(size_str);
                    expected_chunks = (total_bytes + CHUNK_BYTES - 1) / CHUNK_BYTES;
                    have_start = 1;
                    printf("START: total_bytes=%d expected_chunks=%d domain=%s\n",
                           total_bytes, expected_chunks, domain);
                }
            }

            // ---------- DATA packet: d<hex>.iNNNN.e0/e1.suffix ----------
            else if (type == 'd') {
                const char *hex = labels[0] + 1;

                if (!is_hex_str(hex) || (strlen(hex) % 2) != 0) {
                    printf("Invalid hex payload: %s\n", hex);
                } else if (labels[1][0] != 'i' || !is_digits(labels[1] + 1)) {
                    printf("Invalid seq label: %s\n", labels[1]);
                } else if (labels[2][0] != 'e' || !is_digits(labels[2] + 1)) {
                    printf("Invalid end label: %s\n", labels[2]);
                } else {
                    int seq = atoi(labels[1] + 1);
                    int endflag = atoi(labels[2] + 1);

                    if (seq <= 0 || seq >= MAX_CHUNKS) {
                        printf("Seq out of range: %d\n", seq);
                    } else {
                        int hex_len = (int)strlen(hex);
                        int byte_len = hex_len / 2;
                        if (byte_len > CHUNK_BYTES) {
                            printf("Chunk too large: %d bytes\n", byte_len);
                        } else {
                            printf("DATA: seq=%d end=%d domain=%s\n",
                                   seq, endflag, domain);

                            if (!chunks[seq].seen) {
                                chunks[seq].seen = 1;
                                chunks[seq].len = byte_len;
                                for (int i = 0; i < byte_len; i++) {
                                    chunks[seq].data[i] = hex_to_byte(hex + 2*i);
                                }
                            } else {
                                printf("Duplicate seq=%d, ignoring\n", seq);
                            }

                            if (endflag == 1) {
                                if (seq > last_seq) last_seq = seq;
                            }

                            int complete = 0;
                            int final_chunks = 0;

                            if (have_start && expected_chunks > 0) {
                                final_chunks = expected_chunks;
                                complete = 1;
                                for (int i = 1; i <= final_chunks; i++) {
                                    if (!chunks[i].seen) {
                                        complete = 0;
                                        break;
                                    }
                                }
                            } else if (last_seq > 0) {
                                final_chunks = last_seq;
                                complete = 1;
                                for (int i = 1; i <= final_chunks; i++) {
                                    if (!chunks[i].seen) {
                                        complete = 0;
                                        break;
                                    }
                                }
                            }

                            if (complete) {
                                printf("--- ALL CHUNKS PRESENT, WRITING FILE ---\n");
                                FILE *out = fopen("received_file.txt", "wb");
                                if (!out) {
                                    perror("fopen received_file.txt");
                                } else {
                                    for (int i = 1; i <= final_chunks; i++) {
                                        fwrite(chunks[i].data, 1, chunks[i].len, out);
                                    }
                                    fclose(out);
                                    printf("--- SUCCESS: FILE WRITTEN ---\n");
                                }
                                done = 1;
                            }
                        }
                    }
                }
            } else {
                printf("Non-tunnel request: %s\n", domain);
            }
        } else {
            printf("Generic request: %s\n", domain);
        }

        // ---------- DNS response ----------
        unsigned char response[BUF_SIZE];
        memset(response, 0, BUF_SIZE);
        memcpy(response, buffer, n);

        response[2] = 0x81; // QR=1, RD=1
        response[3] = 0x80; // RA=1, no error

        response[6] = 0x00;
        response[7] = 0x01; // ANCOUNT=1

        struct DNS_ANSWER ans;
        ans.name   = htons(0xC00C);
        ans.type   = htons(1);
        ans.class  = htons(1);
        ans.ttl    = htonl(60);
        ans.data_len = htons(4);
        ans.address  = inet_addr("1.2.3.4");

        memcpy(response + n, &ans, sizeof(ans));
        sendto(sockfd, response, n + sizeof(struct DNS_ANSWER), 0,
               (struct sockaddr*)&client, client_len);
    }

    close(sockfd);
    return 0;
}