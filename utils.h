#pragma once

inline void log(std::string_view message) {
  std::cout << "[+] " << message << std::endl;
}
inline void loghex(std::string_view message, DWORD64 x) {
  std::cout << "[+] " << message << std::hex << x << std::endl;
}

