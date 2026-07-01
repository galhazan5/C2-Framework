🛡️ The Mission

## A Hackeriot x BGU Collaboration Project
This project implements a modular, multi-channel Command and Control (C2) framework designed to demonstrate firewall evasion, asynchronous communication, and automated vulnerability scanning. The system coordinates three separate operational channels: an HTTPS channel interacting with the Google Drive API, an HTTPS channel interacting with the OneDrive API, and a stealthy, bidirectional DNS Tunneling channel operating over raw UDP Port 53.


## System Architecture & File Structure

The project environment is divided into the C2 Controller Server (Linux-based compiled C infrastructure) and the Remote Victim Agent (PowerShell execution environment).

##1. C2 Controller Infrastructure (Server-Side)
The server codebase is modularized into three core component files alongside its compiled binary execution configurations:

* c2_core.h: The central configuration header. It defines global system constants (BUF_SIZE, CHUNK_BYTES), layout parameters, structural models for memory blocks (Chunk, C2Agent), live environment keys (tokens, file handles), and shared network state definitions.
* c2_modules.c: The core implementation file containing the listener thread logic. It handles background multi-threading execution (pthread), byte-level DNS query parsing, active API token handling using cURL to interface with cloud providers, and the management of interactive shell data streams.
* main.c: The interactive Command Line Interface (CLI). It manages the operator workspace loop, processes standard navigation instructions (help, listeners, sessions, interact), tracks heartbeats from active targets, and routes operator tasks into active thread queues.

### 2. Payload Management & Agent Templates (Server-Side)
The server references environment configurations to dynamically output single, unified script components during the runtime payload generation phase:
* template_drive.ps1: The template file mapped for the Google Drive environment. It contains parameters for persistent HTTP polling and access credential exchange.
* template_onedrive.ps1: The template file mapped for the OneDrive environment. It handles HTTP asynchronous communication and file-based task staging via Microsoft cloud infrastructure.
* template_dns.ps1: The template file mapped for the network environment. It defines the loop configuration that queries the controller via DNS TXT records for dynamic tasks and pipes system responses outward through multi-packet A record streams.
* script_discovery.ps1: A shared discovery module containing native system profiling implementations. During generation, its logic is injected directly into the active deployment payload header.

### 3. Deployed Victim Components (Target-Side)
The victim workstation runs specific agent modules compiled and exported by the controller engine:
* generated_agent.ps1: The deployed agent configured for Google Cloud infrastructure communication.
* generated_agent_onedrive.ps1: The deployed agent configured for OneDrive infrastructure communication.
* generated_agent_dns.ps1: The standalone agent configured for DNS tunneling. It evaluates incoming data and routes task execution sequences internally.


## Post-Exploitation & WES-NG Engine Integration

The framework implements an automated pipeline for rapid vulnerability discovery:
1. When an active agent transmits diagnostic system parameters via the network (DNS chunks or cloud storage uploads), the controller intercepts the stream and reconstructs the data into received_file.txt.
2. The runtime scanner detects critical system fingerprints (Host Name, OS Name, and hotfix numbers).
3. The server forks a python execution engine process (popen) using WES-NG (Windows Exploit Suggester - Next Generation) via:
   bash
   python3 wes.py received_file.txt --muc-lookup


## Future Improvements

Code Refactoring and Modularity Constraints

Because this framework was developed as a fast-paced academic project focused on proving core defensive evasion concepts (Multi-Channel communication and DNS Tunneling architecture), the baseline server implementation is primarily procedural. While highly functional as a Proof of Concept, the current codebase contains monolithic sections that require additional refinement and clean decoupling.

