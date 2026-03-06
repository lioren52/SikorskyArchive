#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <thread> // For sleep
#include <chrono> // For timing
#include <conio.h> // For _getch() (Password masking)
#include <windows.h> // For colors


void packFile(std::string inPath, std::ofstream& outStream, std::string fileName) {
    std::cout << std::endl;
    std::ifstream inStream(inPath, std::ios::binary);

    inStream.seekg(0, std::ifstream::end);
    long long size = inStream.tellg();
    inStream.seekg(0, std::ifstream::beg);


    if (size == -1) {
        std::cout << "Error opening the file: " << fileName << std::endl;
        return;
    }

    std::cout << "Packing the file: " << fileName << std::endl;
    std::cout << "File Size: " << size << " bytes" << std::endl;

    outStream.seekp(0, std::ifstream::end);

    outStream.write(fileName.c_str(), fileName.length());
    outStream.put('\0');

    outStream.write(reinterpret_cast<char*>(&size), sizeof(size));

    int bufferSize = 1024;
    std::vector<char> buffer(bufferSize);

    while (inStream) {
        inStream.read(buffer.data(), bufferSize);
        std::streamsize bytesRead = inStream.gcount();
        
        if (bytesRead > 0) {
            outStream.write(buffer.data(), bytesRead);
        }
    }
    
    std::cout << "Packed!!!" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
}

void unPack(std::ifstream& inStream, std::string outPath) {

    inStream.seekg(0, std::ifstream::end);
    long long archiveSize = inStream.tellg();
    inStream.seekg(0, std::ifstream::beg);

    std::cout << std::endl;

    if (archiveSize == -1) {
        std::cout << "Error cannot read the file" << std::endl;
        return;
    }

    while (inStream.peek() != EOF && inStream.tellg() < archiveSize) {

        std::string fileName = "";
        char ch;

        while (inStream.get(ch) && ch != '\0') {
            fileName += ch;
        }

        if (!inStream || fileName.empty()) {
            break;
        }

        std::cout << "Un-Packing File: " << fileName << '\n';

        long long Size;
        inStream.read(reinterpret_cast<char*>(&Size), 8);

        std::cout << "Size: " << Size  << " bytes" << std::endl;

        std::filesystem::path file(outPath+fileName);
        if (file.has_parent_path()) {
            std::filesystem::create_directories(file.parent_path());
        }

        std::ofstream outStream(outPath+fileName, std::ios::binary);
        
        int bufferSize = 1024;
        std::vector<char> buffer(bufferSize);

        long long remaining = Size;
        while (remaining > 0) {
            // Determine how much to read: either bufferSize or whatever is left
            long long toRead = (remaining < bufferSize) ? remaining : bufferSize;

            inStream.read(buffer.data(), toRead);
            outStream.write(buffer.data(), toRead);

            remaining -= toRead;
        }

        std::cout << "Saved to: " << outPath+fileName << std::endl;
        std::cout << "--------------------------------------" << std::endl;
    } 
}

std::vector<std::pair<std::string, std::string>> folderIterator(std::string folderPath) {
    std::vector<std::pair<std::string, std::string>> locationList;
    
    for (auto dir_entry : std::filesystem::recursive_directory_iterator(folderPath)) {
        std::string absPath = dir_entry.path().string();
        std::string relPath = std::filesystem::relative(absPath, folderPath).string();
        locationList.emplace_back(absPath, relPath);
    }

    return locationList;
}

void packFolder(std::string folderPath, std::ofstream& outStream) {
    std::vector<std::pair<std::string, std::string>> locationList = folderIterator(folderPath);
    for (std::pair<std::string, std::string> ite : locationList) {
        if (std::filesystem::is_regular_file(ite.first)) {
            packFile(ite.first, outStream, ite.second);
        }
    }
}

void deriveKey(const std::string& password, unsigned char* key) {
    // SHA256 transforms ANY string into exactly 32 bytes of hash
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), key);
}

void generateIV(unsigned char* iv) {
    // Fills the buffer with cryptographically strong random bytes
    if (!RAND_bytes(iv, 16)) {
        std::cerr << "Error generating random IV!" << std::endl;
        exit(1);
    }
}

bool encryptFile(std::string inPath, std::ofstream& outStream, std::string password) {
    std::ifstream inStream(inPath, std::ios::binary);
    if (!inStream) {
        std::cerr << "Could not open input file!" << std::endl;
        return false;
    }

    // 1. Prepare Key and IV
    unsigned char key[32];
    unsigned char iv[16];
    deriveKey(password, key);
    generateIV(iv);

    // 2. Write the IV to the file FIRST (Unencrypted)
    // The decoder will read these first 16 bytes to know how to decrypt the rest.
    outStream.write(reinterpret_cast<char*>(iv), 16);

    // 3. Setup OpenSSL Context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    
    // Initialize encryption engine with AES-256-CBC
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        std::cerr << "Encryption Init failed!" << std::endl;
        return false;
    }

    // 4. The Loop
    const int bufferSize = 4096; // 4KB is optimal
    unsigned char inBuffer[bufferSize];
    unsigned char outBuffer[bufferSize + 16]; // Output can be slightly larger due to padding
    int outLen;

    const std::string testPhrase = "SIKORSKY-CORE-V1";

    if (1 != EVP_EncryptUpdate(ctx, outBuffer, &outLen, reinterpret_cast<const unsigned char*>(testPhrase.c_str()), testPhrase.length())) {
        std::cerr << "Encryption Update failed!" << std::endl;
        return false;
    }
    
    // Write encrypted chunk to disk
    outStream.write(reinterpret_cast<char*>(outBuffer), outLen);



    while (inStream.read(reinterpret_cast<char*>(inBuffer), bufferSize)) {
        int bytesRead = inStream.gcount();
        
        // Encrypt this chunk
        if (1 != EVP_EncryptUpdate(ctx, outBuffer, &outLen, inBuffer, bytesRead)) {
            std::cerr << "Encryption Update failed!" << std::endl;
            return false;
        }
        
        // Write encrypted chunk to disk
        outStream.write(reinterpret_cast<char*>(outBuffer), outLen);
    }

    // Handle the last chunk (if file size wasn't perfectly divisible by 4096)
    int bytesRead = inStream.gcount();
    if (bytesRead > 0) {
         if (1 != EVP_EncryptUpdate(ctx, outBuffer, &outLen, inBuffer, bytesRead)) {
             // Handle error
         }
         outStream.write(reinterpret_cast<char*>(outBuffer), outLen);
    }

    // 5. Finalize (Add Padding)
    if (1 != EVP_EncryptFinal_ex(ctx, outBuffer, &outLen)) {
        std::cerr << "Encryption Final failed!" << std::endl;
        return false;
    }
    outStream.write(reinterpret_cast<char*>(outBuffer), outLen);

    // 6. Cleanup
    EVP_CIPHER_CTX_free(ctx);
    
    std::cout << "Folder Encrypted" << std::endl;
    return true;
}
void setColor(int color) {
    // 7 = White, 12 = Red, 10 = Green, 14 = Yellow
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

bool decryptFile(std::string inPath, std::ofstream& outStream, std::string password) {
    std::ifstream inStream(inPath, std::ios::binary);
    if (!inStream) {
        std::cerr << "Could not open input file: " << inPath << std::endl;
        return false;
    }

    // 1. Recover the IV (Read the first 16 bytes)
    unsigned char iv[16];
    inStream.read(reinterpret_cast<char*>(iv), 16);

    // If file is too short (less than 16 bytes), it's not a valid archive
    if (inStream.gcount() < 16) {
        std::cerr << "File too short to contain IV!" << std::endl;
        return false;
    }

    // 2. Derive the Key (Must match the encryption key exactly)
    unsigned char key[32];
    deriveKey(password, key);

    // 3. Setup OpenSSL Context (Decrypt Mode)
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    
    // Note: We use EVP_DecryptInit_ex here (instead of EncryptInit)
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        std::cerr << "Decryption Init failed!" << std::endl;
        return false;
    }

    // 4. The Loop
    const int bufferSize = 4096;
    unsigned char inBuffer[bufferSize];
    unsigned char outBuffer[bufferSize + 16]; // Output buffer needs wiggle room
    int outLen;
    bool isFirstChunk = true;
    const std::string testPhrase = "SIKORSKY-CORE-V1";

    while (inStream.read(reinterpret_cast<char*>(inBuffer), bufferSize)) {
        int bytesRead = inStream.gcount();
        
        // Decrypt this chunk
        if (1 != EVP_DecryptUpdate(ctx, outBuffer, &outLen, inBuffer, bytesRead)) {
            std::cerr << "Decryption Update failed!" << std::endl;
            return false;
        }
        if (isFirstChunk && outLen > 0) {
            isFirstChunk = false; // Never run this block again
            
            // Read the first bytes to see if they match our check phrase
            std::string decryptedStart(reinterpret_cast<char*>(outBuffer), std::min((int)testPhrase.length(), outLen));
            
            if (decryptedStart != testPhrase) {
                setColor(12); // Red text
                std::cerr << "\n[!] ACCESS DENIED: Incorrect Password or Corrupted Archive!\n" << std::endl;
                setColor(7);  // Reset text
                EVP_CIPHER_CTX_free(ctx);
                return false; // Fail fast and abort!
            }
            
            // It matched! Write the REST of the buffer to the file (skipping over the magic phrase)
            outStream.write(reinterpret_cast<char*>(outBuffer + testPhrase.length()), outLen - testPhrase.length());
        } else {
            // Normal write for all subsequent chunks
            outStream.write(reinterpret_cast<char*>(outBuffer), outLen);
        }
    }

    // Handle the last chunk
    int bytesRead = inStream.gcount();
    if (bytesRead > 0) {
         if (1 != EVP_DecryptUpdate(ctx, outBuffer, &outLen, inBuffer, bytesRead)) {
            // Handle error
         }
         outStream.write(reinterpret_cast<char*>(outBuffer), outLen);
    }

    // 5. Finalize (Remove Padding)
    // CRITICAL: This step verifies the data integrity. 
    // If the password is wrong or data is corrupt, this function returns 0 (False).
    if (1 != EVP_DecryptFinal_ex(ctx, outBuffer, &outLen)) {
        std::cerr << "Decryption Final failed! (Wrong password or corrupted file?)" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    outStream.write(reinterpret_cast<char*>(outBuffer), outLen);

    // 6. Cleanup
    EVP_CIPHER_CTX_free(ctx);
    
    std::cout << "Decrypted: " << inPath << std::endl;
    return true;
}


void clearScreen() {
    system("cls");
}

void printLogo() {
    setColor(12); // Red for "Blacklist" vibe
    std::cout << R"(
  _________.__ __                         __          
 /   _____/|__|  | ______  ___________  |  | _____.__.
 \_____  \ |  |  |/ /  _ \/  _ \_  __ \ |  |/ <   |  |
 /        \|  |    <  <_> (  <_> )  | \/ |    < \___  |
/_______  /|__|__|_ \____/ \____/|__|    |__|_ \/ ____|
        \/         \/                         \/\/     
       >>> ENCRYPTED ARCHIVE PROTOCOL v1.0 <<<
)" << "\n";
    setColor(7); // Reset to White
}

void handleEncryption() {
    std::string folderPath, fileName, filePath, password, pause;
    std::cout << "Folder Path: ";
    std::getline(std::cin, folderPath);
    if (folderPath[0] == '?') {
        folderPath = folderPath.substr(1, folderPath.length()-1);
    }

    std::cout << "Encrypted File Output Name: ";
    std::getline(std::cin, fileName);
    if (fileName[0] == '?') {
        fileName = fileName.substr(1, fileName.length()-1);
    }

    std::cout << "Output file Path: ";
    std::getline(std::cin, filePath);
    if (filePath[0] == '?') {
        filePath = filePath.substr(1, filePath.length()-1);
    }

    std::cout << "Password: ";
    std::getline(std::cin, password);

    std::ofstream temp("temp.dat", std::ios::binary);
    packFolder(folderPath, temp);
    temp.close();

    std::ofstream output(filePath+"\\"+fileName+".sikorsky", std::ios::binary);

    bool succ = encryptFile("temp.dat", output, password);
    output.close();

    std::cout << "Encrypted File saved at: " << filePath+"\\"+fileName+".sikorsky" << std::endl;
    int delStat = std::remove("temp.dat");
    if (delStat) {
        setColor(12);
        std::cout << "CAUTION: ";
        setColor(7);
        std::cout << "Unable to delete the temporary file at: " << std::filesystem::current_path() << std::endl;
    }
    std::cin >> pause;
}

void handleDecryption() {
    std::string folderPath, filePath, password, pause;
    std::cout << "File Path: ";
    std::getline(std::cin, filePath);
    if (filePath[0] == '?') {
        filePath = filePath.substr(1, filePath.length()-1);
    }

    std::cout << "Extraction Location: ";
    std::getline(std::cin, folderPath);
    if (folderPath[0] == '?') {
        folderPath = folderPath.substr(1, folderPath.length()-1);
    }
    if (!folderPath.empty() && folderPath.back() != '/' && folderPath.back() != '\\') {
        folderPath += '\\';
    }

    std::cout << "Password: ";
    std::getline(std::cin, password);

    std::cout << std::endl;

    std::cout << "Decrypting: " << filePath << std::endl;

    std::cout << "Starting Decryption......";
    std::ofstream temp("temp.dat", std::ios::binary);

    bool de = decryptFile(filePath, temp, password);
    temp.close();
    
    if (de) {
        std::cout << "Decryption Successfull......" << std::endl;
        std::cout << "Unpacking....." << std::endl;
        std::ifstream unpack("temp.dat", std::ios::binary);
        unPack(unpack, folderPath);
        unpack.close();
    } else {
        std::cin >> pause;
    }

    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "Files Decrypted at: " << folderPath << std::endl;
    int delStat = std::remove("temp.dat");
    if (delStat) {
        setColor(12);
        std::cout << "CAUTION: ";
        setColor(7);
        std::cout << "Unable to delete the temporary file at: " << std::filesystem::current_path() << std::endl;
    }
    std::cin >> pause;

}

int main() {
    // Set Console Title
    SetConsoleTitleA("SIKORSKY ARCHIVE");

    while (true) {
        clearScreen();
        printLogo();
        
        std::cout << "\n[1] LOCK   (Pack & Encrypt)";
        std::cout << "\n[2] UNLOCK (Decrypt & Unpack)";
        std::cout << "\n[3] ABORT  (Exit)";
        std::cout << "\n\n>> SELECT COMMAND: ";

        char choice;
        std::cin >> choice;
        std::cin.ignore(10000, '\n');

        if (choice == '1') {
            handleEncryption();
        } else if (choice == '2') {
            handleDecryption();
        } else if (choice == '3') {
            setColor(12);
            std::cout << "\n\n>> TERMINATING CONNECTION...\n";
            setColor(7);
            break;
        }
    }
    return 0;
}