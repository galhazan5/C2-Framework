🛡️ The Mission
A Hackeriot x BGU Collaboration Project
This project is a full-stack Command and Control (C2) Framework built from scratch. It demonstrates how a remote attacker 
can exfiltrate sensitive data from a Windows target by abusing the DNS protocol to bypass modern firewall restrictions.

🛠️ The Architecture: How it Works
The framework consists of three main components that work in total synchronization:
The Listener (Server): A custom DNS server written in C (Linux-based). 
It captures UDP traffic on Port 53, extracts Hex-encoded payloads from subdomains, and reconstructs the original data.
The Modular Agent (Client): A post-exploitation "Implant" developed in two parallel versions (PowerShell and Batch) to 
ensure redundancy and stealth.
The DNS Tunnel: A stealthy communication channel that fragments data and hides it within standard DNS queries 
(e.g., d[HEX].i[SEQ].e[IS_LAST].domain.com).

📈 Project Roadmap: From Theory to Tool
The development was executed in strategic milestones, ensuring a deep understanding of each layer of the C2 
infrastructure.

Milestone 1: Theoretical Foundation & Reconnaissance
Before any implementation, we conducted extensive research into the "Trust" relationship of core protocols and Windows
internals.
Protocol Analysis: Deep dive into DNS (UDP 53) and why it remains a primary target for covert communication due to its 
ubiquity in firewalls.
Environment Setup: Configuring the Kali Linux C2 server and Windows 10 victim environments in a controlled virtual lab (VM).
System Discovery: Identifying key Windows artifacts for post-exploitation: Processes, Network states, and NTFS file 
structures.

Milestone 2: Developing the C2 Listener (Server-Side)
Building the "Brain" of the operation using C Programming to handle low-level network communication.
Socket Programming: Implementation of a UDP server to intercept DNS queries.
The Reassembly Engine: Developing the logic to strip DNS headers, extract Hexadecimal payloads from subdomains, and
re-order fragmented chunks based on Sequence IDs.
Data Integrity: Ensuring that the final exfiltrated file matches the source data through end-to-end testing.

Milestone 3: Weaponizing the Agents (Client-Side)
Creating the "Soldiers" that execute the mission. We prioritized Redundancy by developing two parallel versions:
The Modern Agent (PowerShell): Utilizing .NET reflection and object-oriented logic to capture and stream data dynamically.
The Stealth Agent (Batch/CMD): A minimalist implementation designed to evade detection.


💡 Key Insights & Personal Growth
Building this framework from zero was a transformative experience:
Cross-Platform Synchronization: Ensuring a script in Windows "talks" perfectly to a server in C (Linux) required meticulous 
protocol design.
Problem Solving: Debugging why a Batch script couldn't encode Hex correctly taught me how to dive deep into Windows system 
binaries.
Hackeriot Empowerment: This project reflects the high-level technical training provided by the Hackeriot and BGU program 
for women in cyber.


🛡️ Disclaimer
This project is for educational and authorized testing purposes only.
