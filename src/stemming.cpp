#include <string>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

static bool is_consonant(const std::string& w, int i) {
    char c = w[i];
    if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u')
        return false;
    if (c == 'y')
        return (i == 0) ? true : !is_consonant(w, i - 1);
    return true;
}

static int measure(const std::string& w) {
    int m = 0;
    bool in_vowel = false;
    for (size_t i = 0; i < w.size(); ++i) {
        if (!is_consonant(w, i)) {
            in_vowel = true;
        } else if (in_vowel) {
            m++;
            in_vowel = false;
        }
    }
    return m;
}

static bool contains_vowel(const std::string& w) {
    for (size_t i = 0; i < w.size(); ++i)
        if (!is_consonant(w, i))
            return true;
    return false;
}

static bool ends_with(const std::string& w, const std::string& s) {
    return w.size() >= s.size() &&
           w.compare(w.size() - s.size(), s.size(), s) == 0;
}

static bool double_consonant(const std::string& w) {
    int n = w.size();
    return n >= 2 &&
           w[n - 1] == w[n - 2] &&
           is_consonant(w, n - 1);
}

static bool cvc(const std::string& w) {
    int n = w.size();
    if (n < 3) return false;
    return is_consonant(w, n - 1) &&
           !is_consonant(w, n - 2) &&
           is_consonant(w, n - 3) &&
           w[n - 1] != 'w' &&
           w[n - 1] != 'x' &&
           w[n - 1] != 'y';
}


std::string stem(const std::string& input) {
    for (unsigned char c : input)
        if (c > 127)
            return input;

    std::string w = input;
    if (w.size() < 3) return w;

    if (ends_with(w, "sses")) w.resize(w.size() - 2);
    else if (ends_with(w, "ies")) w.resize(w.size() - 2);
    else if (ends_with(w, "ss")) {}
    else if (ends_with(w, "s")) w.resize(w.size() - 1);

    if (ends_with(w, "eed")) {
        std::string stem = w.substr(0, w.size() - 3);
        if (measure(stem) > 0)
            w.resize(w.size() - 1);
    } else if ((ends_with(w, "ed") && contains_vowel(w.substr(0, w.size() - 2))) ||
               (ends_with(w, "ing") && contains_vowel(w.substr(0, w.size() - 3)))) {
        if (ends_with(w, "ed")) w.resize(w.size() - 2);
        else w.resize(w.size() - 3);

        if (ends_with(w, "at") || ends_with(w, "bl") || ends_with(w, "iz"))
            w += "e";
        else if (double_consonant(w) &&
                 !(w.back() == 'l' || w.back() == 's' || w.back() == 'z'))
            w.pop_back();
        else if (measure(w) == 1 && cvc(w))
            w += "e";
    }

    if (ends_with(w, "y") && contains_vowel(w.substr(0, w.size() - 1)))
        w[w.size() - 1] = 'i';

    struct Rule { const char* suf; const char* rep; };
    static Rule step2[] = {
        {"ational", "ate"}, {"tional", "tion"}, {"enci", "ence"},
        {"anci", "ance"}, {"izer", "ize"}, {"abli", "able"},
        {"alli", "al"}, {"entli", "ent"}, {"eli", "e"},
        {"ousli", "ous"}, {"ization", "ize"}, {"ation", "ate"},
        {"ator", "ate"}, {"alism", "al"}, {"iveness", "ive"},
        {"fulness", "ful"}, {"ousness", "ous"}, {"aliti", "al"},
        {"iviti", "ive"}, {"biliti", "ble"}
    };

    for (auto& r : step2) {
        if (ends_with(w, r.suf)) {
            std::string stem = w.substr(0, w.size() - std::strlen(r.suf));
            if (measure(stem) > 0) {
                w = stem + r.rep;
            }
            break;
        }
    }

    if (ends_with(w, "e")) {
        std::string stem = w.substr(0, w.size() - 1);
        if (measure(stem) > 1 || (measure(stem) == 1 && !cvc(stem)))
            w = stem;
    }

    return w;
}

namespace py = pybind11;

PYBIND11_MODULE(stemmer, m) {
    m.def("stem", &stem, "Porter stemmer (English, UTF-8 safe)");
}