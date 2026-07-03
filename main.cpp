#include <chrono>  // For timing
#include <conio.h> // For _getch() (Password masking)
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <thread> // For sleep
#include <vector>
#include <windows.h> // For colors

void packFile(std::string inPath, std::ofstream &outStream,
              std::string fileName) {
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

  outStream.write(reinterpret_cast<char *>(&size), sizeof(size));

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

void unPack(std::ifstream &inStream, std::string outPath) {

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
    inStream.read(reinterpret_cast<char *>(&Size), 8);

    std::cout << "Size: " << Size << " bytes" << std::endl;

    if (fileName.find("..") != std::string::npos ||
        (!fileName.empty() &&
         (fileName.front() == '/' || fileName.front() == '\\'))) {
      std::cerr << "Security violation: Invalid path traversal in archive: "
                << fileName << std::endl;
      break;
    }

    std::filesystem::path file(outPath + fileName);
    if (file.has_parent_path()) {
      std::filesystem::create_directories(file.parent_path());
    }

    std::ofstream outStream(outPath + fileName, std::ios::binary);

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

    std::cout << "Saved to: " << outPath + fileName << std::endl;
    std::cout << "--------------------------------------" << std::endl;
  }
}

std::vector<std::pair<std::string, std::string>>
folderIterator(std::string folderPath) {
  std::vector<std::pair<std::string, std::string>> locationList;

  for (auto dir_entry :
       std::filesystem::recursive_directory_iterator(folderPath)) {
    std::string absPath = dir_entry.path().string();
    std::string relPath =
        std::filesystem::relative(absPath, folderPath).string();
    locationList.emplace_back(absPath, relPath);
  }

  return locationList;
}

void packFolder(std::string folderPath, std::ofstream &outStream) {
  std::vector<std::pair<std::string, std::string>> locationList =
      folderIterator(folderPath);
  for (std::pair<std::string, std::string> ite : locationList) {
    if (std::filesystem::is_regular_file(ite.first)) {
      packFile(ite.first, outStream, ite.second);
    }
  }
}

void deriveKey(const std::string &password, const unsigned char *salt,
               unsigned char *key) {
  if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(), salt, 16, 600000,
                        EVP_sha256(), 32, key) != 1) {
    std::cerr << "Key derivation failed!" << std::endl;
    exit(1);
  }
}

void generateRandomBytes(unsigned char *buffer, int length) {
  if (!RAND_bytes(buffer, length)) {
    std::cerr << "Error generating random bytes!" << std::endl;
    exit(1);
  }
}

bool encryptFile(std::string inPath, std::ofstream &outStream,
                 std::string password) {
  std::ifstream inStream(inPath, std::ios::binary);
  if (!inStream) {
    std::cerr << "Could not open input file!" << std::endl;
    return false;
  }

  // 1. Prepare Salt, Key and IV
  unsigned char salt[16];
  generateRandomBytes(salt, 16);
  unsigned char key[32];
  deriveKey(password, salt, key);
  unsigned char iv[12]; // GCM standard IV size is 12 bytes
  generateRandomBytes(iv, 12);

  // 2. Write Salt and IV to the file FIRST (Unencrypted)
  outStream.write(reinterpret_cast<char *>(salt), 16);
  outStream.write(reinterpret_cast<char *>(iv), 12);

  // 3. Setup OpenSSL Context
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  // Initialize encryption engine with AES-256-GCM
  if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv)) {
    std::cerr << "Encryption Init failed!" << std::endl;
    return false;
  }

  // 4. The Loop
  const int bufferSize = 4096; // 4KB is optimal
  unsigned char inBuffer[bufferSize];
  unsigned char outBuffer[bufferSize +
                          16]; // Output can be slightly larger due to padding
  int outLen;

  while (inStream.read(reinterpret_cast<char *>(inBuffer), bufferSize)) {
    int bytesRead = inStream.gcount();

    // Encrypt this chunk
    if (1 != EVP_EncryptUpdate(ctx, outBuffer, &outLen, inBuffer, bytesRead)) {
      std::cerr << "Encryption Update failed!" << std::endl;
      return false;
    }

    // Write encrypted chunk to disk
    outStream.write(reinterpret_cast<char *>(outBuffer), outLen);
  }

  // Handle the last chunk
  int bytesRead = inStream.gcount();
  if (bytesRead > 0) {
    if (1 != EVP_EncryptUpdate(ctx, outBuffer, &outLen, inBuffer, bytesRead)) {
      std::cerr << "Encryption Update failed!" << std::endl;
      return false;
    }
    outStream.write(reinterpret_cast<char *>(outBuffer), outLen);
  }

  // 5. Finalize
  if (1 != EVP_EncryptFinal_ex(ctx, outBuffer, &outLen)) {
    std::cerr << "Encryption Final failed!" << std::endl;
    return false;
  }
  outStream.write(reinterpret_cast<char *>(outBuffer), outLen);

  // 6. Get the GCM authentication tag and append it to the file
  unsigned char tag[16];
  if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)) {
    std::cerr << "Failed to get GCM tag!" << std::endl;
    return false;
  }
  outStream.write(reinterpret_cast<char *>(tag), 16);

  // 7. Cleanup
  EVP_CIPHER_CTX_free(ctx);

  std::cout << "Folder Encrypted" << std::endl;
  return true;
}
void setColor(int color) {
  // 7 = White, 12 = Red, 10 = Green, 14 = Yellow
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

bool decryptFile(std::string inPath, std::ofstream &outStream,
                 std::string password) {
  std::ifstream inStream(inPath, std::ios::binary);
  if (!inStream) {
    std::cerr << "Could not open input file: " << inPath << std::endl;
    return false;
  }

  // Get file size to find the tag at the end
  inStream.seekg(0, std::ios::end);
  long long fileSize = inStream.tellg();

  // Salt (16) + IV (12) + Tag (16) = 44 bytes minimum
  if (fileSize < 44) {
    std::cerr << "File too small to be a valid archive!" << std::endl;
    return false;
  }

  // Read the authentication tag from the end
  unsigned char tag[16];
  inStream.seekg(fileSize - 16, std::ios::beg);
  inStream.read(reinterpret_cast<char *>(tag), 16);

  // Reset stream and read salt & IV
  inStream.seekg(0, std::ios::beg);

  // 1. Recover the Salt and IV
  unsigned char salt[16];
  unsigned char iv[12];
  inStream.read(reinterpret_cast<char *>(salt), 16);
  inStream.read(reinterpret_cast<char *>(iv), 12);

  // 2. Derive the Key (Must match the encryption key exactly)
  unsigned char key[32];
  deriveKey(password, salt, key);

  // 3. Setup OpenSSL Context (Decrypt Mode)
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

  // Initialize decryption engine with AES-256-GCM
  if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, iv)) {
    std::cerr << "Decryption Init failed!" << std::endl;
    return false;
  }

  // 4. The Loop
  const int bufferSize = 4096;
  unsigned char inBuffer[bufferSize];
  unsigned char outBuffer[bufferSize + 16]; // Output buffer needs wiggle room
  int outLen;

  long long remainingCiphertext =
      fileSize - 16 - 16 - 12; // fileSize - tag - salt - iv

  while (remainingCiphertext > 0) {
    long long toRead =
        (remainingCiphertext < bufferSize) ? remainingCiphertext : bufferSize;
    inStream.read(reinterpret_cast<char *>(inBuffer), toRead);
    int bytesRead = inStream.gcount();

    // Decrypt this chunk
    if (1 != EVP_DecryptUpdate(ctx, outBuffer, &outLen, inBuffer, bytesRead)) {
      std::cerr << "Decryption Update failed!" << std::endl;
      EVP_CIPHER_CTX_free(ctx);
      return false;
    }

    outStream.write(reinterpret_cast<char *>(outBuffer), outLen);
    remainingCiphertext -= bytesRead;
  }

  // Set the expected tag before finalizing
  if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag)) {
    std::cerr << "Failed to set GCM tag!" << std::endl;
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  // 5. Finalize (Verifies Tag)
  // CRITICAL: This step verifies the data integrity.
  // If the password is wrong or data is corrupt, this function returns 0
  // (False).
  if (1 != EVP_DecryptFinal_ex(ctx, outBuffer, &outLen)) {
    setColor(12);
    std::cerr
        << "\n[!] ACCESS DENIED: Incorrect Password or Corrupted Archive!\n"
        << std::endl;
    setColor(7);
    EVP_CIPHER_CTX_free(ctx);
    return false;
  }

  outStream.write(reinterpret_cast<char *>(outBuffer), outLen);

  // 6. Cleanup
  EVP_CIPHER_CTX_free(ctx);

  std::cout << "Decrypted: " << inPath << std::endl;
  return true;
}

std::string maskPassword() {
  std::string password = "";
  char ch;

  // 13 is the ASCII value for the Enter key ('\r')
  while ((ch = _getch()) != 13) {
    // 8 is the ASCII value for the Backspace key ('\b')
    if (ch == 8) {
      if (password.length() > 0) {
        password.pop_back(); // Remove last character from the string
        // Move cursor back, print a space to overwrite the *, then move cursor
        // back again
        std::cout << "\b \b";
      }
    }
    // Ignore special keys (like arrows) or Ctrl+C to keep it simple
    // 32 to 126 covers all standard printable ASCII characters
    else if (ch >= 32 && ch <= 126) {
      password += ch;
      std::cout << '*';
    }
  }
  std::cout << std::endl; // Move to the next line after they press Enter
  return password;
}

void clearScreen() { system("cls"); }

int shredFile(const std::string &path) {
  std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
  if (file) {
    file.seekg(0, std::ios::end);
    long long size = file.tellg();
    file.seekp(0, std::ios::beg);

    const int bufSize = 4096;
    std::vector<char> zeros(bufSize, 0);
    long long remaining = size;
    while (remaining > 0) {
      long long toWrite = (remaining < bufSize) ? remaining : bufSize;
      file.write(zeros.data(), toWrite);
      remaining -= toWrite;
    }
    file.close();
  }
  return std::remove(path.c_str());
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
    folderPath = folderPath.substr(1, folderPath.length() - 1);
  }

  std::cout << "Encrypted File Output Name: ";
  std::getline(std::cin, fileName);
  if (fileName[0] == '?') {
    fileName = fileName.substr(1, fileName.length() - 1);
  }

  std::cout << "Output file Path: ";
  std::getline(std::cin, filePath);
  if (filePath[0] == '?') {
    filePath = filePath.substr(1, filePath.length() - 1);
  }

  std::cout << "Password: ";
  password = maskPassword();

  std::cout << "Confirm Password: ";
  std::string confirm = maskPassword();

  if (password != confirm) {
    setColor(12);
    std::cout << "\n[!] Error: Passwords do not match. Aborting.\n";
    setColor(7);
    std::cin >> pause;
    return;
  }

  std::ofstream temp("temp.dat", std::ios::binary);
  packFolder(folderPath, temp);
  temp.close();

  std::ofstream output(filePath + "\\" + fileName + ".sikorsky",
                       std::ios::binary);

  bool succ = encryptFile("temp.dat", output, password);
  output.close();

  std::cout << "Encrypted File saved at: "
            << filePath + "\\" + fileName + ".sikorsky" << std::endl;
  int delStat = shredFile("temp.dat");
  if (delStat) {
    setColor(12);
    std::cout << "CAUTION: ";
    setColor(7);
    std::cout << "Unable to shred the temporary file at: "
              << std::filesystem::current_path() << "\\temp.dat" << std::endl;
  }
  std::cin >> pause;
  return;
}

void handleDecryption() {
  std::string folderPath, filePath, password, pause;
  std::cout << "File Path: ";
  std::getline(std::cin, filePath);
  if (filePath[0] == '?') {
    filePath = filePath.substr(1, filePath.length() - 1);
  }

  std::cout << "Extraction Location: ";
  std::getline(std::cin, folderPath);
  if (folderPath[0] == '?') {
    folderPath = folderPath.substr(1, folderPath.length() - 1);
  }
  if (!folderPath.empty() && folderPath.back() != '/' &&
      folderPath.back() != '\\') {
    folderPath += '\\';
  }

  std::cout << "Password: ";
  password = maskPassword();

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
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "Files Decrypted at: " << folderPath << std::endl;
    int delStat = shredFile("temp.dat");
    if (delStat) {
      setColor(12);
      std::cout << "CAUTION: ";
      setColor(7);
      std::cout << "Unable to shred the temporary file at: "
                << std::filesystem::current_path() << "\\temp.dat" << std::endl;
    }
    std::cin >> pause;
  } else {
    shredFile("temp.dat");
    std::cin >> pause;
    return;
  }
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