#include "configr.h"

#include <getopt.h>
#include <cstdlib>
#include <cerrno>
#include <climits>

static std::size_t idx(Key k) noexcept {
  return static_cast<std::size_t>(k);
}

Configr& Configr::instance() noexcept {
  static Configr inst;
  return inst;
}

Configr::Configr() noexcept {
  reset_defaults();
}

void Configr::reset_defaults() noexcept {
  values_[idx(Key::version)] = false;
  values_[idx(Key::help)]    = false;
  values_[idx(Key::dbhost)]  = std::string{};
  values_[idx(Key::dbname)]  = std::string{};
  values_[idx(Key::dbusr)]   = std::string{};
  values_[idx(Key::dbpass)]  = std::string{};
  values_[idx(Key::port)]    = 0;
  values_[idx(Key::ai)]       = std::string{};
  values_[idx(Key::monitor)] = false;
  values_[idx(Key::log)]     = std::string{};

  supplied_.fill(false);
  ok_ = true;
  error_.clear();
}

bool Configr::invalidate(int argc, char** argv) noexcept {
  return instance().parse(argc, argv);
}

bool Configr::has(Key k) const noexcept {
  if (idx(k) >= supplied_.size()) return false;
  return supplied_[idx(k)];
}

const Value& Configr::value(Key k) const noexcept {
  static const Value kEmpty = std::monostate{};
  const auto i = idx(k);
  if (i >= values_.size()) return kEmpty;
  return values_[i];
}

template<Key K>
void Configr::set_user_value(typename KeyType<K>::type v) noexcept {
  values_[idx(K)] = std::move(v);
  supplied_[idx(K)] = true;
}

template<Key K>
const typename KeyType<K>::type& Configr::get_impl() const noexcept {
  // values_ always contains the correct type for each key because reset_defaults()
  // establishes it and set_user_value<K> preserves it.
  return std::get<typename KeyType<K>::type>(values_[idx(K)]);
}

// Explicit instantiations for get_impl used across
//  translation units (optional but tidy)

template const bool&        Configr::get_impl<Key::version>() const noexcept;
template const bool&        Configr::get_impl<Key::help>() const noexcept;
template const std::string& Configr::get_impl<Key::dbhost>() const noexcept;
template const std::string& Configr::get_impl<Key::dbname>() const noexcept;
template const std::string& Configr::get_impl<Key::dbusr>() const noexcept;
template const std::string& Configr::get_impl<Key::dbpass>() const noexcept;
template const std::string& Configr::get_impl<Key::ai>() const noexcept;
template const int&         Configr::get_impl<Key::port>() const noexcept;
template const bool&        Configr::get_impl<Key::monitor>() const noexcept;
template const std::string& Configr::get_impl<Key::log>() const noexcept;

static bool parse_int_noexcept(const char* s, int& out) noexcept {
  if (!s || !*s) {
      return false;
  }

  errno = 0;
  char* end = nullptr;

  long v = std::strtol(s, &end, 10);

  if (errno != 0) {
     return false;
  }
  if (end == s || *end != '\0') {
     return false;
  }
  if (v < INT_MIN || v > INT_MAX) {
     return false;
  }
  out = static_cast<int>(v);
  return true;
}

bool Configr::parse(int argc, char** argv) noexcept {
  reset_defaults();

  opterr = 0;
  optind = 1;

  static option longopts[] = {
    {"version", no_argument,       nullptr, 'v'},
    {"help",    no_argument,       nullptr, 'h'},
    {"dbhost",  required_argument, nullptr, 'H'},
    {"dbname",  required_argument, nullptr, 'd'},
    {"dbusr",   required_argument, nullptr, 'u'},
    {"dbpass",  required_argument, nullptr, 'p'},
    {"ai",      required_argument, nullptr, 'A'},
    {"port",    required_argument, nullptr, 'P'},
    {"monitor", no_argument,       nullptr, 'M'},
    {"log",     required_argument, nullptr, 'L'},
    {nullptr,   0,                 nullptr, 0}
  };

  const char* optstring = ":hvMu:d:p:P:L:H";

  int c = 0;
  int longindex = 0;

  while ((c = getopt_long(argc, argv, optstring, longopts, &longindex)) != -1) {
    switch (c) {
      case 'v':
        set_user_value<Key::version>(true);
        break;
      case 'h':
        set_user_value<Key::help>(true);
        break;
      case 'H':
        set_user_value<Key::dbhost>(std::string(optarg ? optarg : ""));
        break;
      case 'd':
        set_user_value<Key::dbname>(std::string(optarg ? optarg : ""));
        break;
      case 'u':
        set_user_value<Key::dbusr>(std::string(optarg ? optarg : ""));
        break;
      case 'A':
        set_user_value<Key::ai>(std::string(optarg ? optarg : "dsl"));
        break;
      case 'p':
        set_user_value<Key::dbpass>(std::string(optarg ? optarg : ""));
        break;
      case 'P': {
        int port = 0;
        if (!parse_int_noexcept(optarg, port)) {
          ok_ = false;
          error_ = "invalid integer for --port/-P";
          return false;
        }
        set_user_value<Key::port>(port);
        break;
      }
      case 'M':
        set_user_value<Key::monitor>(true);
        break;
      case 'L':
        set_user_value<Key::log>(std::string(optarg ? optarg : ""));
        break;

      case ':': // missing required argument
                // (because optstring starts with ':')
        ok_ = false;
        error_ = "missing required argument for an option";
        return false;
      case '?': // unknown option
        error_ = "case for ?";
        break;
      default:
        ok_ = false;
        error_ = "unknown option";
        return false;
    }
  }

  ok_ = true;
  return true;
}
