#include <SPI.h>
#include <SD.h>

const int chipSelect = 4;

// === FUNCTION: Initialize SD ===
bool initSD() {
  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return false;
  }
  Serial.println("SD card ready.");
  return true;
}

// === FUNCTION: Check if SD has any files ===
bool hasFilesInRoot() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory!");
    return false;
  }
  File entry = root.openNextFile();
  bool hasFiles = (entry != 0);
  entry.close();
  root.close();
  return hasFiles;
}

// === FUNCTION: List all files ===
void listFiles(const char *dirname = "/", uint8_t levels = 0) {
  File dir = SD.open(dirname);
  if (!dir) {
    Serial.println("Failed to open directory.");
    return;
  }

  Serial.print("Listing files in: ");
  Serial.println(dirname);

  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    Serial.print("  ");
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      if (levels) listFiles(entry.name(), levels - 1);
    } else {
      Serial.print("\tSize: ");
      Serial.println(entry.size());
    }
    entry.close();
  }
  dir.close();
}

// === FUNCTION: Delete a single file ===
bool deleteFile(const char *filename) {
  if (SD.exists(filename)) {
    if (SD.remove(filename)) {
      Serial.print("Deleted file: ");
      Serial.println(filename);
      return true;
    } else {
      Serial.print("Failed to delete: ");
      Serial.println(filename);
      return false;
    }
  } else {
    Serial.print("File not found: ");
    Serial.println(filename);
    return false;
  }
}

// === FUNCTION: Delete all files in a folder ===
void deleteAllFilesInFolder(const char *folder) {
  File dir = SD.open(folder);
  if (!dir) {
    Serial.println("Failed to open folder!");
    return;
  }

  Serial.print(" Clearing folder: ");
  Serial.println(folder);

  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    Serial.print("  Deleting: ");
    Serial.println(entry.name());
    SD.remove(entry.name());
    entry.close();
  }
  dir.close();
  Serial.println("All files deleted.");
}

// === SETUP ===
void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  if (!initSD()) return;

  Serial.println("\n=== SD Card Manager Ready ===");
  Serial.println("Commands:");
  Serial.println("  list            → List files");
  Serial.println("  delete <file>   → Delete a specific file");
  Serial.println("  clear <folder>  → Delete all files in a folder (use / for root)");
  Serial.println("  check           → Check if SD has files");
  Serial.println("----------------------------------");
}

// === LOOP: Wait for Serial commands ===
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("list")) {
      listFiles("/");
    } 
    else if (input.startsWith("delete ")) {
      String filename = input.substring(7);
      filename.trim();
      deleteFile(filename.c_str());
    } 
    else if (input.startsWith("clear ")) {
      String folder = input.substring(6);
      folder.trim();
      deleteAllFilesInFolder(folder.c_str());
    } 
    else if (input.equalsIgnoreCase("check")) {
      if (hasFilesInRoot()) Serial.println("SD card has files.");
      else Serial.println("No files found on SD card.");
    } 
    else {
      Serial.println("Unknown command.");
    }

    Serial.println("----------------------------------");
  }
}
