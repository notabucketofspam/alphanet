#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <vector>
#include "AlphaSign.h"

// Helper to convert strings to lowercase for easier argument parsing
std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}

void printUsage(const char* programName) {
  std::cout << "AlphaSign Command Line Interface\n"
    << "Usage: " << programName << " <port> \"<message>\" [color] [mode]\n\n"
    << "Arguments:\n"
    << "  port      Windows: COM3  |  Linux: /dev/ttyUSB0\n"
    << "  message   The text to display (wrap in quotes)\n"
    << "  color     red, green, amber, dimred, dimgreen, brown, orange,\n"
    << "            yellow, rainbow1, rainbow2, mix, auto (default: auto)\n"
    << "  mode      hold, rotate, flash, scroll, rollup, rolldown, wipeup,\n"
    << "            wipedown, wipeleft, wiperight (default: hold)\n\n"
    << "Examples:\n"
#ifdef _WIN32
    << "  " << programName << " COM3 \"SYSTEM ONLINE\" green rotate\n";
#else
    << "  " << programName << " /dev/ttyUSB0 \"SYSTEM ONLINE\" green rotate\n";
#endif
}

void parseEscapeSequences(std::string& str) {
  // 1. Handle literal "\n" or "\r" typed in Windows CMD / basic quotes
  size_t pos = 0;
  while ((pos = str.find("\\n", pos)) != std::string::npos) {
    str.replace(pos, 2, "\x0D");
    pos += 1;
  }

  pos = 0;
  while ((pos = str.find("\\r", pos)) != std::string::npos) {
    str.replace(pos, 2, "\x0D");
    pos += 1;
  }

  // 2. Handle literal "\p" for New Page (0x0C)
  pos = 0;
  while ((pos = str.find("\\p", pos)) != std::string::npos) {
    str.replace(pos, 2, "\x0C");
    pos += 1;
  }

  // 3. Handle actual Line Feed bytes (0x0A) from Bash $'...' interpolation
  // Normalizes them to the hardware's expected 0x0D.
  std::replace(str.begin(), str.end(), '\n', '\x0D');
}

AlphaSign::SignColor parseColor(const std::string& colorStr) {
  std::string c = toLower(colorStr);
  if (c == "red")      return AlphaSign::SignColor::Red;
  if (c == "green")    return AlphaSign::SignColor::Green;
  if (c == "amber")    return AlphaSign::SignColor::Amber;
  if (c == "dimred")   return AlphaSign::SignColor::DimRed;
  if (c == "dimgreen") return AlphaSign::SignColor::DimGreen;
  if (c == "brown")    return AlphaSign::SignColor::Brown;
  if (c == "orange")   return AlphaSign::SignColor::Orange;
  if (c == "yellow")   return AlphaSign::SignColor::Yellow;
  if (c == "rainbow1") return AlphaSign::SignColor::Rainbow1;
  if (c == "rainbow2") return AlphaSign::SignColor::Rainbow2;
  if (c == "mix")      return AlphaSign::SignColor::ColorMix;
  return AlphaSign::SignColor::AutoColor;
}

AlphaSign::DisplayMode parseMode(const std::string& modeStr) {
  std::string m = toLower(modeStr);
  if (m == "rotate")    return AlphaSign::DisplayMode::Rotate;
  if (m == "flash")     return AlphaSign::DisplayMode::Flash;
  if (m == "scroll")    return AlphaSign::DisplayMode::Scroll;
  if (m == "rollup")    return AlphaSign::DisplayMode::RollUp;
  if (m == "rolldown")  return AlphaSign::DisplayMode::RollDown;
  if (m == "wipeup")    return AlphaSign::DisplayMode::WipeUp;
  if (m == "wipedown")  return AlphaSign::DisplayMode::WipeDown;
  if (m == "wipeleft")  return AlphaSign::DisplayMode::WipeLeft;
  if (m == "wiperight") return AlphaSign::DisplayMode::WipeRight;
  return AlphaSign::DisplayMode::Hold; // Default
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printUsage(argv[0]);
    return 1;
  }

  std::string portName = argv[1];
  std::string message = argv[2];

  parseEscapeSequences(message);

#ifdef _WIN32
  // Windows API requires \\.\COMx for ports, especially COM10 and above.
  // If the user just typed "COM3", silently fix it for them.
  std::string lowerPort = toLower(portName);
  if (lowerPort.length() >= 4 && lowerPort.substr(0, 3) == "com" && lowerPort.find("\\\\.\\") == std::string::npos) {
    portName = "\\\\.\\" + portName;
  }
#endif

  // Setup configuration with defaults
  AlphaSign::SignConfig config;
  config.typeCode = 'Z';   // Broadcast to all signs connected to the bus
  config.isPriority = false;
  config.position = AlphaSign::DisplayPosition::Fill;

  // Parse optional Color argument
  if (argc >= 4) {
    config.color = parseColor(argv[3]);
  }

  // Parse optional Mode argument
  if (argc >= 5) {
    config.mode = parseMode(argv[4]);
  }

  std::cout << "Connecting to " << argv[1] << "...\n"; // Print what the user typed
  AlphaSign::SerialPort port(portName);                // Use the safely formatted string

  if (!port.isValid()) {
    std::cerr << "Error: Could not open serial port.\n";
    return 1;
  }

  if (!config.isPriority) {
    std::cout << "Allocating memory...\n";
    std::vector<AlphaSign::FileConfig> memoryLayout = {
      // label, type, isLocked, size, dotsH, dotsW, dotsColor, startTime, stopTime
      { 'A', AlphaSign::FileType::Text, false, 256, 0, 0, "2000", "FF", "00" }
    };
    auto configPacket = AlphaSign::createConfigPacket(memoryLayout, config.typeCode);
    port.write(configPacket);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  std::cout << "Sending message: \"" << message << "\"\n";
  auto packet = AlphaSign::createTextMessagePacket(message, config);

  if (port.write(packet)) {
    std::cout << "Transmission successful.\n";
  } else {
    std::cerr << "Error: Failed to write to serial port.\n";
    return 1;
  }

  return 0;
}
