<p align="center">
  <br>
  <h1 align="center">SIKORSKY ARCHIVE</h1>
</p>

<p align="center">
  <i>"Secrets are not meant to be hidden. They are meant to be secured."</i>
</p>

<br>

<p align="center">
  <b>SYSTEMS LEVEL ENCRYPTION // AES-256-CBC // CUSTOM PACKING PROTOCOL</b>
</p>

<hr>

<h3>:: SYSTEM OVERVIEW</h3>

<p>
Sikorsky Archive is a high-performance systems tool designed to pack entire directory structures into opaque, cryptographically secure binary blobs. Built in C++17, it implements a custom streaming protocol for file packing and leverages the OpenSSL EVP interface for hardware-accelerated encryption.
</p>

<p>
This is not a wrapper. It is a raw implementation of file stream manipulation, block cipher chaining, and manual memory management.
</p>

<br>

<h3>:: TECHNICAL SPECIFICATIONS</h3>

| Component | Specification |
| :--- | :--- |
| **Core Logic** | C++17 (Standard) |
| **Cryptography** | OpenSSL (EVP Interface) |
| **Algorithm** | AES-256-CBC (Cipher Block Chaining) |
| **Hashing** | SHA-256 (Key Derivation) |
| **Interface** | Windows Console API (TUI) |
| **Output** | `.sikorsky` (Custom Binary Format) |

<br>

<h3>:: CAPABILITIES</h3>

**[ + ] Custom Packing Protocol** Implements a proprietary binary format that serializes file metadata, relative paths, and binary content into a single continuous stream before encryption.

**[ + ] Military-Grade Cryptography** Secured using AES-256. Random Initialization Vectors (IV) are generated for every session to prevent pattern analysis. The IV is embedded in the file header for stateless decryption.

**[ + ] Ephemeral Processing** Intermediate unencrypted data (`temp.dat`) exists only during the transformation window and is shredded from the disk immediately upon completion.

**[ + ] Zero Dependency** Statically linked against OpenSSL. No DLL hell. No external installers. Just a single, portable executable.

<hr>

<h3>:: BUILD PROTOCOL</h3>

<p>This project requires a C++17 compliant compiler (MinGW-w64 recommended) and OpenSSL static libraries.</p>

<b>Manual Compilation Command:</b>
<pre>
g++ main.cpp -o sikorsky.exe ^
    -I"./include" ^
    -L"./lib" ^
    -lssl -lcrypto -lws2_32 -lgdi32 -lcrypt32
</pre>

<br>

<h3>:: OPERATION MANUAL</h3>

The tool operates via a Text User Interface (TUI).

#### // LOCK MODE (Pack & Encrypt)
1. The system scans the target folder recursively.
2. A binary stream is generated containing the file structure.
3. The stream is encrypted using AES-256-CBC into a `.sikorsky` archive.
4. The source stream is destroyed.

#### // UNLOCK MODE (Decrypt & Unpack)
1. The system validates the archive integrity and IV.
2. AES-256-CBC decryption restores the binary stream.
3. The unpacker reconstructs the original directory tree.

<br>

<hr>

<p align="center">
  <b>DISCLAIMER</b><br>
  This software is provided "as is". AES-256 is mathematically secure; if you lose your password, your data is irretrievable.<br>
  <i>Use responsibly.</i>
</p>

<p align="center">
  Built by <b>Arul Saini</b>
</p>
