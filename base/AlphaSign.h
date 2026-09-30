#ifndef ALPHASIGN_H
#define ALPHASIGN_H

#include <vector>
#include <string>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

# if defined(__linux__) && defined(_WIN32)
#error "__linux__ and _WIN32 are both defined. you gotta choose one or the other, pal."
#endif

namespace AlphaSign {

  // --- Enums ---

  enum class SignColor : char {
    Red = '1',
    Green = '2',
    Amber = '3',
    DimRed = '4',
    DimGreen = '5',
    Brown = '6',
    Orange = '7',
    Yellow = '8',
    Rainbow1 = '9',
    Rainbow2 = 'A',
    ColorMix = 'B',
    AutoColor = 'C'
  };

  enum class DisplayMode : char {
    Rotate = 'a',
    Hold = 'b',
    Flash = 'c',
    RollUp = 'e',
    RollDown = 'f',
    RollLeft = 'g',
    RollRight = 'h',
    WipeUp = 'i',
    WipeDown = 'j',
    WipeLeft = 'k',
    WipeRight = 'l',
    Scroll = 'm',
    AutoMode = 'o',
    RollIn = 'p',
    RollOut = 'q',
    WipeIn = 'r',
    WipeOut = 's'
  };

  enum class DisplayPosition : char {
    Middle = ' ',  // 20H
    Top = '"',  // 22H
    Bottom = '&',  // 26H
    Fill = '0'   // 30H
  };

  enum class FileType : char {
    Text = 'A',
    String = 'B',
    Dots = 'D'
  };

  namespace SpecialFunc {
    extern const std::string SetMemory;
    extern const std::string SoftReset;
    extern const std::string TimeOfDay;
    extern const std::string GeneralInfo;
    extern const std::string SerialError;
    extern const std::string SpeakerTone;
    extern const std::string LargeDotsMem;
  }

  // --- Data Structures ---

  struct SignConfig {
    char typeCode = 'Z';
    std::string signAddress = "00";
    bool isPriority = false;
    char fileLabel = 'A';
    DisplayPosition position = DisplayPosition::Fill;
    DisplayMode mode = DisplayMode::Hold;
    SignColor color = SignColor::Amber;
  };

  struct FileConfig {
    char label;
    FileType type;
    bool isLocked;
    uint16_t sizeBytes = 0;
    uint8_t dotsHeight = 0;
    uint8_t dotsWidth = 0;
    std::string dotsColor = "2000"; // "1000"=mono, "2000"=tricolor, "4000"=8-color
    std::string startTime = "FF";
    std::string stopTime = "00";
  };

  // --- Packet Generators ---

  std::vector<uint8_t> createConfigPacket(const std::vector<FileConfig>& files, char typeCode = 'Z', const std::string& address = "00");

  std::vector<uint8_t> createTextMessagePacket(const std::string& message, const SignConfig& config = SignConfig{});

  std::vector<uint8_t> createStringMessagePacket(char fileLabel, const std::string& data, char typeCode = 'Z', const std::string& address = "00");

  std::vector<uint8_t> createWriteSpecialFunctionPacket(const std::string& label, const std::string& data = "", char typeCode = 'Z', const std::string& address = "00");

  std::vector<uint8_t> createReadSpecialFunctionPacket(const std::string& label, char typeCode = 'Z', const std::string& address = "00");

  std::vector<uint8_t> createSmallDotsPicturePacket(char fileLabel, uint8_t height, uint8_t width, const std::vector<std::string>& rows, char typeCode = 'Z', const std::string& address = "00");

  std::vector<uint8_t> createLargeDotsPicturePacket(const std::string& fileName, uint16_t height, uint16_t width, const std::vector<std::string>& rows, char typeCode = 'Z', const std::string& address = "00");

  // --- Windows Serial Port Wrapper ---

#ifdef _WIN32
  class SerialPort {
  public:
    explicit SerialPort(const std::string& portName, DWORD baudRate = CBR_9600);
    ~SerialPort();

    bool isValid() const;
    bool write(const std::vector<uint8_t>& data);
    std::string readResponse(DWORD timeoutMs = 1000);

  private:
    HANDLE m_handle;
  };
#else
  class SerialPort {
  public:
    // Use /dev/ttyUSB0 or similar for the portName
    explicit SerialPort(const std::string& portName, int baudRate = 9600);
    ~SerialPort();

    bool isValid() const;
    bool write(const std::vector<uint8_t>& data);
    std::string readResponse(int timeoutMs = 1000);

  private:
    int m_fd;
  };
#endif

} // namespace AlphaSign

#endif // ALPHASIGN_H
