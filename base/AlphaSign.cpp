#ifdef __linux__
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#endif

#include "AlphaSign.h"
#include <sstream>
#include <iomanip>
#include <iostream>

namespace AlphaSign {

  // --- Constants Definitions ---
  namespace SpecialFunc {
    const std::string SetMemory = "$";
    const std::string SoftReset = ",";
    const std::string TimeOfDay = " ";
    const std::string GeneralInfo = "\"";
    const std::string SerialError = "*";
    const std::string SpeakerTone = "(";
    const std::string LargeDotsMem = "8";
  }

  // --- Internal Helpers (Hidden from other files) ---
  namespace {
    std::string calculateChecksum(const std::vector<uint8_t>& packet, size_t stxIndex, size_t etxIndex) {
      uint16_t sum = 0;
      for (size_t i = stxIndex; i <= etxIndex; ++i) {
        sum += packet[i];
      }
      std::stringstream ss;
      ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << sum;
      return ss.str();
    }

    std::string toHex2(uint8_t val) {
      std::stringstream ss;
      ss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (int)val;
      return ss.str();
    }

    std::string padName9(const std::string& name) {
      std::string padded = name;
      if (padded.length() > 9) padded = padded.substr(0, 9);
      while (padded.length() < 9) padded += ' ';
      return padded;
    }

    std::string compressPixelRow(const std::string& rawRow) {
      std::string compressed;
      size_t i = 0;
      while (i < rawRow.length()) {
        char currentColor = rawRow[i];
        size_t count = 1;
        while (i + count < rawRow.length() && rawRow[i + count] == currentColor && count < 256) {
          count++;
        }
        if (count > 3) {
          compressed += (char)0x11; // <DC1> Run-length compression trigger
          std::stringstream hexCount;
          hexCount << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (count - 1);
          compressed += hexCount.str();
          compressed += currentColor;
        } else {
          compressed.append(count, currentColor);
        }
        i += count;
      }
      return compressed;
    }

    std::string buildMemoryConfigData(const std::vector<FileConfig>& files) {
      std::stringstream ss;
      for (const auto& file : files) {
        ss << file.label << static_cast<char>(file.type) << (file.isLocked ? 'L' : 'U');
        if (file.type == FileType::Dots) {
          ss << toHex2(file.dotsHeight) << toHex2(file.dotsWidth) << file.dotsColor;
        } else {
          ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << file.sizeBytes
            << file.startTime << file.stopTime;
        }
      }
      return ss.str();
    }

    void appendHeader(std::vector<uint8_t>& packet, char typeCode, const std::string& address) {
      for (int i = 0; i < 10; ++i) packet.push_back(0x00); // Sync
      packet.push_back(0x01); // <SOH>
      packet.push_back(typeCode);
      packet.push_back(address[0]);
      packet.push_back(address[1]);
    }

    void appendChecksumAndEOT(std::vector<uint8_t>& packet, size_t stxIndex) {
      size_t etxIndex = packet.size();
      packet.push_back(0x03); // <ETX>
      std::string checksum = calculateChecksum(packet, stxIndex, etxIndex);
      for (char c : checksum) packet.push_back(c);
      packet.push_back(0x04); // <EOT>
    }
  } // end anonymous namespace

  // --- Packet Generators ---

  std::vector<uint8_t> createConfigPacket(const std::vector<FileConfig>& files, char typeCode, const std::string& address) {
    std::vector<uint8_t> packet;
    appendHeader(packet, typeCode, address);
    size_t stxIndex = packet.size();
    packet.push_back(0x02); // <STX>
    packet.push_back('E');
    packet.push_back('$');
    std::string configData = buildMemoryConfigData(files);
    for (char c : configData) packet.push_back(c);
    appendChecksumAndEOT(packet, stxIndex);
    return packet;
  }

  std::vector<uint8_t> createTextMessagePacket(const std::string& message, const SignConfig& config) {
    std::vector<uint8_t> packet;
    appendHeader(packet, config.typeCode, config.signAddress);
    size_t stxIndex = packet.size();
    packet.push_back(0x02); // <STX>
    packet.push_back('A');
    packet.push_back(config.isPriority ? '0' : config.fileLabel);
    packet.push_back(0x1B); // <ESC>
    packet.push_back(static_cast<uint8_t>(config.position));
    packet.push_back(static_cast<uint8_t>(config.mode));
    packet.push_back(0x1C); // <US> Color code
    packet.push_back(static_cast<uint8_t>(config.color));
    for (char c : message) packet.push_back(c);
    appendChecksumAndEOT(packet, stxIndex);
    return packet;
  }

  std::vector<uint8_t> createStringMessagePacket(char fileLabel, const std::string& data, char typeCode, const std::string& address) {
    std::vector<uint8_t> packet;
    appendHeader(packet, typeCode, address);
    size_t stxIndex = packet.size();
    packet.push_back(0x02); // <STX>
    packet.push_back('G');
    packet.push_back(fileLabel);
    for (char c : data) packet.push_back(c);
    appendChecksumAndEOT(packet, stxIndex);
    return packet;
  }

  std::vector<uint8_t> createWriteSpecialFunctionPacket(const std::string& label, const std::string& data, char typeCode, const std::string& address) {
    std::vector<uint8_t> packet;
    appendHeader(packet, typeCode, address);
    size_t stxIndex = packet.size();
    packet.push_back(0x02); // <STX>
    packet.push_back('E');
    for (char c : label) packet.push_back(c);
    for (char c : data)  packet.push_back(c);
    appendChecksumAndEOT(packet, stxIndex);
    return packet;
  }

  std::vector<uint8_t> createReadSpecialFunctionPacket(const std::string& label, char typeCode, const std::string& address) {
    std::vector<uint8_t> packet;
    appendHeader(packet, typeCode, address);
    packet.push_back(0x02); // <STX>
    packet.push_back('F');
    for (char c : label) packet.push_back(c);
    packet.push_back(0x04); // <EOT>
    return packet;
  }

  std::vector<uint8_t> createSmallDotsPicturePacket(char fileLabel, uint8_t height, uint8_t width, const std::vector<std::string>& rows, char typeCode, const std::string& address) {
    std::vector<uint8_t> packet;
    appendHeader(packet, typeCode, address);
    size_t stxIndex = packet.size();
    packet.push_back(0x02); // <STX>
    packet.push_back('I');
    packet.push_back(fileLabel);
    std::string hHex = toHex2(height);
    std::string wHex = toHex2(width);
    packet.push_back(hHex[0]); packet.push_back(hHex[1]);
    packet.push_back(wHex[0]); packet.push_back(wHex[1]);
    for (const std::string& row : rows) {
      for (char pixel : row) packet.push_back(pixel);
      packet.push_back(0x0D); // <CR>
    }
    appendChecksumAndEOT(packet, stxIndex);
    return packet;
  }

  std::vector<uint8_t> createLargeDotsPicturePacket(const std::string& fileName, uint16_t height, uint16_t width, const std::vector<std::string>& rows, char typeCode, const std::string& address) {
    std::vector<uint8_t> packet;
    appendHeader(packet, typeCode, address);
    size_t stxIndex = packet.size();
    packet.push_back(0x02); // <STX>
    packet.push_back('M');
    std::string paddedName = padName9(fileName);
    for (char c : paddedName) packet.push_back(c);
    std::stringstream hw;
    hw << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << height
      << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << width;
    for (char c : hw.str()) packet.push_back(c);
    for (const std::string& row : rows) {
      std::string processedRow = compressPixelRow(row);
      for (char pixel : processedRow) packet.push_back(pixel);
      packet.push_back(0x0D); // <CR>
    }
    appendChecksumAndEOT(packet, stxIndex);
    return packet;
  }

  // --- Windows Serial Port Wrapper Implementation ---

#ifdef _WIN32
  SerialPort::SerialPort(const std::string& portName, DWORD baudRate) {
    m_handle = CreateFileA(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_handle == INVALID_HANDLE_VALUE) {
      std::cerr << "Failed to open serial port: " << portName << "\n";
      return;
    }
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(dcb);
    GetCommState(m_handle, &dcb);
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(m_handle, &dcb);
  }

  SerialPort::~SerialPort() {
    if (isValid()) CloseHandle(m_handle);
  }

  bool SerialPort::isValid() const {
    return m_handle != INVALID_HANDLE_VALUE;
  }

  bool SerialPort::write(const std::vector<uint8_t>& data) {
    if (!isValid()) return false;
    DWORD bytesWritten = 0;
    return WriteFile(m_handle, data.data(), static_cast<DWORD>(data.size()), &bytesWritten, NULL) && bytesWritten == data.size();
  }

  std::string SerialPort::readResponse(DWORD timeoutMs) {
    if (!isValid()) return "";
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadTotalTimeoutConstant = timeoutMs;
    SetCommTimeouts(m_handle, &timeouts);
    std::string response;
    char buffer[128];
    DWORD bytesRead = 0;
    while (ReadFile(m_handle, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
      for (DWORD i = 0; i < bytesRead; ++i) {
        response += buffer[i];
        if (buffer[i] == 0x04) return response; // <EOT>
      }
    }
    return response;
  }
#else
  SerialPort::SerialPort(const std::string& portName, int baudRate) {
    // Open the serial port file descriptor:
    // O_RDWR   = Read/Write
    // O_NOCTTY = Don't make this the controlling terminal
    // O_SYNC   = Synchronous writes
    m_fd = ::open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);

    if (m_fd < 0) {
      std::cerr << "Failed to open serial port: " << portName << "\n";
      return;
    }

    struct termios tty;
    if (tcgetattr(m_fd, &tty) != 0) {
      std::cerr << "Error getting termios attributes\n";
      return;
    }

    // We only support 9600 for this wrapper, but termios maps it via B9600
    speed_t speed = B9600;
    if (baudRate == 19200) speed = B19200;
    else if (baudRate == 38400) speed = B38400;

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    // Alpha Protocol Requirements: 8 Data bits, No Parity, 1 Stop bit
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit characters
    tty.c_cflag &= ~PARENB;                     // No parity bit
    tty.c_cflag &= ~CSTOPB;                     // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                    // No hardware flow control
    tty.c_cflag |= (CLOCAL | CREAD);            // Ignore modem controls, enable reading

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);     // Disable software flow control (XON/XOFF)
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable special input processing

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input mode (no line buffering)
    tty.c_oflag &= ~OPOST;                          // Raw output mode

    // Blocking behavior: non-blocking reads (handled by select() later)
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0) {
      std::cerr << "Error setting termios attributes\n";
    }
  }

  SerialPort::~SerialPort() {
    if (isValid()) {
      ::close(m_fd);
    }
  }

  bool SerialPort::isValid() const {
    return m_fd >= 0;
  }

  bool SerialPort::write(const std::vector<uint8_t>& data) {
    if (!isValid()) return false;

    // standard POSIX write
    ssize_t bytesWritten = ::write(m_fd, data.data(), data.size());
    return bytesWritten == static_cast<ssize_t>(data.size());
  }

  std::string SerialPort::readResponse(int timeoutMs) {
    if (!isValid()) return "";

    std::string response;
    char buffer[128];

    fd_set read_fds;
    struct timeval timeout;

    while (true) {
      FD_ZERO(&read_fds);
      FD_SET(m_fd, &read_fds);

      // Convert milliseconds to seconds and microseconds
      timeout.tv_sec = timeoutMs / 1000;
      timeout.tv_usec = (timeoutMs % 1000) * 1000;

      // Wait for data to become available (or timeout)
      int ret = ::select(m_fd + 1, &read_fds, NULL, NULL, &timeout);

      if (ret > 0 && FD_ISSET(m_fd, &read_fds)) {
        ssize_t bytesRead = ::read(m_fd, buffer, sizeof(buffer));
        if (bytesRead > 0) {
          for (ssize_t i = 0; i < bytesRead; ++i) {
            response += buffer[i];
            if (buffer[i] == 0x04) {
              return response; // Break early if we hit <EOT>
            }
          }
        } else {
          break; // Read error or EOF
        }
      } else {
        break; // select() timed out or failed
      }
    }
    return response;
  }
#endif

} // namespace AlphaSign
