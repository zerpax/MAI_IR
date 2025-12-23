#include <iostream>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py=pybind11;

bool is_word_char(uint32_t codepoint) {
    if ((codepoint >= 'a' && codepoint <= 'z') ||
        (codepoint >= 'A' && codepoint <= 'Z') ||
        (codepoint >= '0' && codepoint <= '9'))
        return true;
    if (codepoint == '\'' || codepoint == '-' || codepoint == '$') 
        return true;
    // Cyrillic
    if (codepoint >= 0x0400 && codepoint <= 0x04FF)
        return true;
    // Spanish supplement
    if (codepoint >= 0x00C0 && codepoint <= 0x00FF)
        return true;
    
    return false;
}

uint32_t next_utf8_char(const std::string &s, size_t &i) {
    unsigned char c = s[i];
    uint32_t codepoint = 0;

    if (c < 0x80) {             // 1-byte ASCII
        codepoint = c;
        i += 1;
    } else if ((c & 0xE0) == 0xC0) { // 2-byte UTF-8
        codepoint = ((s[i] & 0x1F) << 6) | (s[i+1] & 0x3F);
        i += 2;
    } else if ((c & 0xF0) == 0xE0) { // 3-byte UTF-8
        codepoint = ((s[i] & 0x0F) << 12) |
                    ((s[i+1] & 0x3F) << 6) |
                    (s[i+2] & 0x3F);
        i += 3;
    } else if ((c & 0xF8) == 0xF0) { // 4-byte UTF-8
        codepoint = ((s[i] & 0x07) << 18) |
                    ((s[i+1] & 0x3F) << 12) |
                    ((s[i+2] & 0x3F) << 6) |
                    (s[i+3] & 0x3F);
        i += 4;
    } else {
        // Invalid byte, skip
        codepoint = 0;
        i += 1;
    }

    return codepoint;
}

std::string utf8_char_to_string(uint32_t codepoint) {
    std::string out;
    if (codepoint <= 0x7F) {
        out += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        out += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        out += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
        out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
    return out;
}


std::vector<std::string> tokenize(const std::string &text) {
    std::vector<std::string> tokens;
    std::string token;
    size_t i = 0;

    while (i < text.size()) {
        uint32_t codepoint = next_utf8_char(text, i);

        if (codepoint <= 127)
            codepoint = std::tolower(static_cast<unsigned char>(codepoint));

        if (is_word_char(codepoint)) {
            token += utf8_char_to_string(codepoint);
        } 
        else {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        }
    }

    if (!token.empty())
        tokens.push_back(token);

    return tokens;
}

PYBIND11_MODULE(tokenizer, m, py::mod_gil_not_used()) {
    m.def("tokenize", &tokenize, "tokenizer");
}