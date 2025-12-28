#include "util.h"
#include <sstream>
#include <ctime>
#include <iomanip>
#include <random>
#include <algorithm>
#include <cctype>

std::string trim(const std::string &s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }

    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

char owner_for_username(const std::string &u)
{
    if (u == "alice") return 'A';
    if (u == "bob") return 'B';
    return 0;
}

std::string now_iso()
{
    std::time_t t = std::time(nullptr);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    return std::string(buf);
}

bool starts_with(const std::string &s, const std::string &p)
{
    return s.rfind(p, 0) == 0;
}

std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return s;
}

std::string rand_hex_64()
{
    static const char hex[] = "0123456789abcdef";
    std::string s;
    s.reserve(64);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    for (int i = 0; i < 64; ++i) {
        s += hex[dis(gen)];
    }
    return s;
}

std::vector<std::string> split_ws(const std::string &s)
{
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (ss >> item) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim)
{
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::string escape_json(const std::string &s) {
    std::ostringstream o;
    for (auto c : s) {
        switch (c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= c && c <= '\x1f') {
                o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
            } else {
                o << c;
            }
        }
    }
    return o.str();
}
