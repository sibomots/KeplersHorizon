#ifndef __KH_CONFIGR_MANAGER_H__
#define __KH_CONFIGR_MANAGER_H__

#include <cstddef>
#include <iostream>
#include <array>
#include <string>
#include <variant>
#include <vector>

// Semantic configuration keys (no spelling variants like -h vs --help)
enum class Key : std::size_t {
  version,
  help,
  dbhost,
  dbname,
  dbusr,
  dbpass,
  ai,
  port,
  monitor,
  log,
  Count
};



// Variant value type (runtime access)
using Value = std::variant<
  std::monostate,
  bool,
  int,
  std::string,
  std::vector<std::string>
>;

// Compile-time key -> value type mapping
template<Key K> struct KeyType;

template <> struct KeyType<Key::version> { using type = bool; };
template <> struct KeyType<Key::help>    { using type = bool; };
template <> struct KeyType<Key::dbhost>  { using type = std::string; };
template <> struct KeyType<Key::dbname>  { using type = std::string; };
template <> struct KeyType<Key::dbusr>   { using type = std::string; };
template <> struct KeyType<Key::dbpass>  { using type = std::string; };
template <> struct KeyType<Key::ai>      { using type = std::string; };
template <> struct KeyType<Key::port>    { using type = int; };
template <> struct KeyType<Key::monitor> { using type = bool; };
template <> struct KeyType<Key::log>     { using type = std::string; };


class Configr {
public:
  static Configr& instance() noexcept;

  // Re-parse argv and replace the singleton contents.
  // Returns true on success; false if parse errors occurred.
  // Does not throw.
  static bool invalidate(int argc, char** argv) noexcept;

  // Parse into this instance (called by invalidate).
  // Public for tests.
  bool parse(int argc, char** argv) noexcept;

  // True if user supplied a value for key (vs default).
  bool has(Key k) const noexcept;

  // Summary report
  void summary()
  {
     std::cout << "Configuration summary:" << std::endl;
     if (get<Key::help>()) {
        std::cout << "help requested\n";
     }
     if (get<Key::version>()) {
        std::cout << "version requested\n";
     }
     if (get<Key::monitor>()) {
        std::cout << "monitor requested\n";
     }
     if (has<Key::dbhost>()) {
        std::cout << "dbhost: " << get<Key::dbhost>() << "\n";
     }
     if (has<Key::dbname>()) {
        std::cout << "dbname: " << get<Key::dbname>() << "\n";
     }
     if (has<Key::dbusr>()) {
        std::cout << "dbusr: " << get<Key::dbusr>() << "\n";
     }
     if (has<Key::ai>()) {
        std::cout << "ai: " << get<Key::ai>() << "\n";
     }
     if (has<Key::port>()) {
       std::cout << "port: " << get<Key::port>() << "\n";
     }
     if (has<Key::log>()) {
       std::cout << "log: " << get<Key::log>() << "\n";
     }
     std::cout << std::endl;
  }

  template<Key K>
  bool has() const noexcept {
    return has(K);
  }

  // Typed accessor by compile-time key (never throws).
  // If user did not supply, returns the default value.
  template<Key K>
  const typename KeyType<K>::type& get() const noexcept {
    return get_impl<K>();
  }

  // Variant accessor by runtime key (never throws).
  // Always returns a Value that matches the key's 
  // declared type (or monostate if unknown).
  const Value& value(Key k) const noexcept;

  // Parse status
  bool ok() const noexcept { return ok_; }
  const std::string& error() const noexcept { return error_; }

private:
  Configr() noexcept;
  void reset_defaults() noexcept;

  template<Key K>
  const typename KeyType<K>::type& get_impl() const noexcept;

  template<Key K>
  void set_user_value(typename KeyType<K>::type v) noexcept;

  // Storage
  std::array<Value, static_cast<std::size_t>(Key::Count)> values_{};
  std::array<bool,  static_cast<std::size_t>(Key::Count)> supplied_{};

  bool ok_{true};
  std::string error_;
};

#endif

