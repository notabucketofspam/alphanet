#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <thread>
#include <chrono>
#include "AlphaSign.h"

// Helper to convert strings to lowercase for easier argument parsing
std::string toLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}

// Replaces literal escape sequences and normalizes OS newlines
void parseEscapeSequences(std::string& str) {
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
  pos = 0;
  while ((pos = str.find("\\p", pos)) != std::string::npos) {
    str.replace(pos, 2, "\x0C");
    pos += 1;
  }
  std::replace(str.begin(), str.end(), '\n', '\x0D');
}

void printUsage(const char* programName) {
  std::cout << "AlphaSign Command Line Interface\n"
    << "Usage: " << programName << " [options] <port> \"<message>\"\n\n"
    << "Arguments:\n"
    << "  port              Windows: COM3  |  Linux: /dev/ttyUSB0\n"
    << "  message           The text to display (wrap in quotes).\n"
    << "                    Use \\n for a New Line, and \\p for a New Page.\n\n"
    << "Options:\n"
    << "  -p, --priority    Bypass memory saving for instant, temporary display.\n"
    << "  -c, --color <val> red, green, amber, dimred, dimgreen, brown, orange,\n"
    << "                    yellow, rainbow1, rainbow2, mix, auto (default: auto)\n"
    << "  -m, --mode <val>  hold, rotate, flash, scroll, rollup, rolldown, wipeup,\n"
    << "                    wipedown, wipeleft, wiperight (default: hold)\n\n"
    << "Examples:\n"
#ifdef _WIN32
    << "  " << programName << " COM3 \"STORE HOURS\" -c amber\n"
    << "  " << programName << " -p COM3 \"SYSTEM FAULT\" -c red -m flash\n";
#else
    << "  " << programName << " /dev/ttyUSB0 \"STORE HOURS\" -c amber\n"
    << "  " << programName << " -p /dev/ttyUSB0 \"SYSTEM FAULT\" -c red -m flash\n";
#endif
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
  return AlphaSign::DisplayMode::Hold;
}

int main(int argc, char* argv[]) {
  bool usePriority = false;
  std::string colorArg = "";
  std::string modeArg = "";
  std::vector<std::string> args;

  // Parse flags and collect positional arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-p" || arg == "--priority") {
      usePriority = true;
    } else if ((arg == "-c" || arg == "--color") && i + 1 < argc) {
      colorArg = argv[++i]; // Grab the next argument as the value
    } else if ((arg == "-m" || arg == "--mode") && i + 1 < argc) {
      modeArg = argv[++i];  // Grab the next argument as the value
    } else {
      // Unrecognized flags are treated as positional arguments (port and message)
      args.push_back(arg);
    }
  }

  // We still require exactly 2 positional arguments: port and message
  if (args.size() < 2) {
    printUsage(argv[0]);
    return 1;
  }

  std::string portName = args[0];
  std::string message = args[1];

  parseEscapeSequences(message);

#ifdef _WIN32
  std::string lowerPort = toLower(portName);
  if (lowerPort.length() >= 4 && lowerPort.substr(0, 3) == "com" && lowerPort.find("\\\\.\\") == std::string::npos) {
    portName = "\\\\.\\" + portName;
  }
#endif

  AlphaSign::SignConfig config;
  config.typeCode = 'Z';
  config.isPriority = usePriority;
  config.position = AlphaSign::DisplayPosition::Fill;

  if (!colorArg.empty()) config.color = parseColor(colorArg);
  if (!modeArg.empty()) config.mode = parseMode(modeArg);

  std::cout << "Connecting to " << args[0] << "...\n";
  AlphaSign::SerialPort port(portName);

  if (!port.isValid()) {
    std::cerr << "Error: Could not open serial port.\n";
    return 1;
  }

  if (!usePriority) {
    std::cout << "Allocating memory...\n";
    std::vector<AlphaSign::FileConfig> memoryLayout = {
        { 'A', AlphaSign::FileType::Text, false, 256 }
    };
    auto configPacket = AlphaSign::createConfigPacket(memoryLayout, config.typeCode);
    port.write(configPacket);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  std::cout << (usePriority ? "Sending priority alert: \"" : "Saving message: \"") << message << "\"\n";
  auto packet = AlphaSign::createTextMessagePacket(message, config);

  if (port.write(packet)) {
    std::cout << "Transmission successful.\n";
  } else {
    std::cerr << "Error: Failed to write to serial port.\n";
    return 1;
  }

  return 0;
}
