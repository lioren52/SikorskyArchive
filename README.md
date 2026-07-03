# SikorskyArchive

<p align="center">
  <h1 align="center">SIKORSKY ARCHIVE</h1>
</p>

<p align="center">
  <i>"Secrets are not meant to be hidden. They are meant to be secured."</i>
</p>

<p align="center">
  <b>C++17 • OpenSSL EVP • AES-256-GCM • Custom Binary Archive Format</b>
</p>

---

## Overview

SikorskyArchive is a systems programming project written in **C++17** that packs entire directory trees into a custom binary archive before encrypting them using authenticated encryption with **AES-256-GCM**.

Rather than relying on existing archive formats such as ZIP or TAR, the project implements its own binary serialization protocol for recursively storing directory structures, file metadata, and file contents. The encrypted archive can later be authenticated, decrypted, and reconstructed while preserving the original directory hierarchy.

The project was built to explore low-level file I/O, binary serialization, filesystem traversal, streaming data processing, and modern cryptography using the OpenSSL EVP interface.

---

# Architecture

```
Input Directory
       │
       ▼
Recursive File Traversal
       │
       ▼
Custom Archive Serializer
(relative path + metadata + file bytes)
       │
       ▼
PBKDF2 Key Derivation
       │
       ▼
AES-256-GCM Encryption
       │
       ▼
.sikorsky Archive
```

During extraction:

```
.sikorsky Archive
       │
       ▼
Authentication Tag Verification
       │
       ▼
AES-256-GCM Decryption
       │
       ▼
Archive Parser
       │
       ▼
Directory Reconstruction
```

---

# Features

## Custom Binary Archive Format

Designed a custom archive format that serializes

* Relative file paths
* File metadata
* File sizes
* Raw binary file contents

into a continuous binary stream before encryption.

Unlike conventional archive utilities, the archive layout is implemented entirely from scratch.

---

## Recursive Directory Reconstruction

Recursively traverses directory trees during packing and reconstructs the complete hierarchy during extraction while preserving relative paths.

---

## Authenticated Encryption

Uses the OpenSSL EVP interface to perform authenticated encryption with

* AES-256-GCM
* Random per-session salt
* Random IV generation
* Authentication tag verification

Any modification to the archive or an incorrect password causes authentication failure during decryption.

---

## Password-Based Key Derivation

Encryption keys are derived from the user password using

* PBKDF2
* SHA-256
* 600,000 iterations

This avoids directly using user passwords as encryption keys.

---

## Streaming File Processing

Processes archive data using buffered I/O instead of loading entire files into memory, allowing large folders to be processed efficiently.

---

## Secure Intermediate File Handling

Intermediate plaintext archives are securely overwritten before deletion to reduce data remanence after encryption or extraction.

---

## Archive Extraction Safety

Implements path traversal validation during extraction to reject malicious archive entries attempting to escape the destination directory.

---

# Technical Highlights

* C++17
* OpenSSL EVP API
* AES-256-GCM
* PBKDF2 (SHA-256)
* Recursive filesystem traversal
* Custom binary serialization protocol
* Streaming file I/O
* Windows Console API
* Manual buffer management

---

# Build

Requirements

* C++17 compiler
* MinGW-w64 (recommended)
* OpenSSL static libraries

Compile using

```bash
g++ main.cpp -o sikorsky.exe ^
    -I"./include" ^
    -L"./lib" ^
    -lssl -lcrypto -lws2_32 -lgdi32 -lcrypt32
```

---

# Usage

## Encrypt

1. Select **LOCK**
2. Choose a directory
3. Enter an output location
4. Enter a password
5. A `.sikorsky` archive is generated

---

## Decrypt

1. Select **UNLOCK**
2. Choose a `.sikorsky` archive
3. Enter the password
4. Choose an extraction directory
5. Original directory structure is restored

---

# Project Motivation

This project was built as an exercise in systems programming to understand how archive utilities work internally.

Instead of relying on existing libraries for packaging, the archive format, serialization logic, filesystem traversal, and extraction pipeline were implemented manually. The project also explores authenticated encryption through OpenSSL's EVP interface while emphasizing low-level control over binary data and file streams.

---

# Future Improvements

* Eliminate the intermediate plaintext archive by streaming directly into the encryption pipeline
* Parallelize encryption and decryption for improved throughput
* Cross-platform support (Linux/macOS)
* Refactor into modular components (`archive`, `crypto`, `filesystem`, `cli`)
* CMake build system
* Archive compression before encryption
* Archive format versioning

---

# Disclaimer

This project is intended for educational and systems programming purposes.

Loss of the encryption password makes encrypted archives unrecoverable.

---

Built by **Arul Saini**
