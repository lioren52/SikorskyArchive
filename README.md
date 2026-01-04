# SIKORSKY ARCHIVE

> "There are no secrets. Just truths that haven't been revealed yet."

Sikorsky Archive is a high-performance systems tool designed to pack entire directory structures into opaque, cryptographically secure binary blobs. Built in C++17, it implements a custom streaming protocol for file packing and leverages OpenSSL's EVP interface for hardware-accelerated AES-256-CBC encryption.

This is not a wrapper for a zip tool. It is a raw implementation of file stream manipulation and block cipher chaining.

---

[ SYSTEM CAPABILITIES ]

+ CUSTOM PACKING PROTOCOL
  Implements a proprietary binary format (.sikorsky) that serializes file metadata, relative paths, and binary content into a single continuous stream before encryption.

+ MILITARY-GRADE CRYPTOGRAPHY
  Secured using AES-256-CBC (Cipher Block Chaining). Keys are derived via SHA-256 hashing of the user passphrase. Random Initialization Vectors (IV) are generated for every session to prevent pattern analysis.

+ EPHEMERAL PROCESSING
  Intermediate unencrypted data (temp.dat) exists only during the transformation window and is shredded from the disk immediately upon completion.

+ ZERO DEPENDENCY
  Statically linked against OpenSSL. No DLL hell. No external installers. Just a single, portable executable.

---

[ BUILD PROTOCOL ]

This project requires a C++17 compliant compiler (MinGW-w64 recommended) and OpenSSL static libraries.

1. PREREQUISITES
   - MinGW-w64 (g++)
   - OpenSSL Development Libraries (Headers + .a files)

2. COMPILATION
   You must manually link the OpenSSL libraries and the Windows system dependencies (ws2_32, gdi32, crypt32).

   g++ main.cpp -o sikorsky.exe 
     -I"path/to/openssl/include" 
     -L"path/to/openssl/lib" 
     -lssl -lcrypto -lws2_32 -lgdi32 -lcrypt32

---

[ OPERATION MANUAL ]

The tool operates via a Text User Interface (TUI). 

// MODE 1: LOCK (Pack & Encrypt)
   1. The system scans the target folder recursively.
   2. A custom binary stream is generated containing the file structure.
   3. The stream is encrypted using AES-256-CBC into a .sikorsky archive.
   4. The source stream is destroyed.

   Example Input:
   > Target: E:\Projects\Classified
   > Output: E:\Vault\Project_Alpha.sikorsky
   > Passkey: [HIDDEN]

// MODE 2: UNLOCK (Decrypt & Unpack)
   1. The system validates the archive integrity and IV.
   2. AES-256-CBC decryption restores the binary stream.
   3. The custom unpacker reconstructs the original directory tree.

   Example Input:
   > Source: E:\Vault\Project_Alpha.sikorsky
   > Output: E:\Restored_Data
   > Passkey: [HIDDEN]

---

[ DISCLAIMER ]

This software is provided "as is", without warranty of any kind. 
AES-256 is mathematically secure, but if you forget your password, your data is mathematically unrecoverable. 
There is no "Forgot Password" button. 

Use responsibly.

---

[ AUTHOR ]
Built by Arul Saini.
