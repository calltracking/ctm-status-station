#include "url_utils.h"

char from_hex(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return 0;
}

char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

String url_encode(String str) {
  String encoded = "";
  for (size_t i = 0; i < str.length(); i++) {
    char c = str[i];
    if ((c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      encoded += '%';
      encoded += to_hex((c >> 4) & 0x0F);
      encoded += to_hex(c & 0x0F);
    }
  }
  return encoded;
}
