#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>

#include <regex>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <chrono>
#include <typeinfo>
#include <type_traits>
#include <functional>
#include <utility>

#include <nx/config.h>

namespace nx {

using strings = std::vector<std::string>;

NX_API
bool interactive(void);
NX_API
bool debugged(void);
NX_API
void set_progress(std::size_t x, std::size_t n, std::size_t w = 50);
NX_API
void clear_progress(std::size_t w = 50);

NX_API
std::string home_directory(void);
NX_API
bool exists(const std::string& path);
NX_API
bool directory_exists(const std::string& path);
NX_API
bool file_exists(const std::string& path);
NX_API
bool touch_file(const std::string& path);
NX_API
bool truncate_file(const std::string& path, std::size_t size);
NX_API
bool copy_file(const std::string& from, const std::string& to);
NX_API
bool write_file(const std::string& file, const char* buffer, std::size_t size);

NX_API
uint64_t file_size(const std::string& path);
NX_API
uint64_t
file_size_if_exists(const std::string& path);
NX_API
std::string
slurp(const std::string& path);
NX_API
bool mkpath(const std::string& path);
NX_API
unsigned long rmpath(const std::string& path);
NX_API
bool rmfile(const std::string& path);
NX_API
bool mkfilepath(const std::string& path);
NX_API
void rename(const std::string& from_path, const std::string& to_path);
NX_API
std::string fullpath(const std::string& rel);
NX_API
std::string fullpath(const std::string& dir, const std::string& file);
NX_API
std::string basename(const std::string& path, const std::string& ext = "");
NX_API
std::string dirname(const std::string& path);
NX_API
unsigned long long human_readable_size(const std::string& expr);
NX_API
std::string human_readable_size(
    uint64_t size,
    const std::string& user_unit = "B"
);
NX_API
std::string human_readable_file_size(const std::string& file);
NX_API
std::string human_readable_rate(
    uint64_t count,
    uint64_t millisecs,
    const std::string& user_unit = "B"
);

NX_API
std::string
lc(const std::string& s);

NX_API
std::string
plural(
    const std::string& base,
    const std::string& p,
    std::size_t count
);

NX_API
bool has_spaces(const std::string& s);

NX_API
bool has_spaces(const char* s);

NX_API
void split(const std::string& re, const std::string& expr, strings& list);

NX_API
std::string
subst(const std::string& s, const std::string& re, const std::string& what);

NX_API
std::string
clean_path(const std::string& path);

NX_API
void chomp(std::string& s);

NX_API
std::string
quote(const std::string& s);

NX_API
void unquote(std::string& s);

NX_API
strings glob(const std::string& expr);

NX_API
bool match(const std::string& expr, const std::string& re);

NX_API
bool
match(const std::string& expr, const std::string& re, std::smatch& m);

NX_API
bool
extract_delimited(
    std::string& from,
    std::string& to,
    unsigned char delim = ';',
    unsigned char quote = '\0'
);

NX_API
std::string demangle_type_name(const std::string& mangled);

template <typename T>
std::size_t
enum_index(const T& t)
{ return static_cast<std::underlying_type_t<T>>(t); }

template <typename T>
inline
std::string
type_info(const T& t)
{ return demangle_type_name(typeid(t).name()); }

template <typename T>
inline
std::string
type_info(void)
{ return demangle_type_name(typeid(T).name()); }

inline
strings
split(const std::string& re, const std::string& expr)
{
    strings list;
    split(re, expr, list);

    return list;
}

template <typename T>
std::string
join(const std::string& sep, const std::vector<T>& v)
{
    std::ostringstream oss;

    if (!v.empty()) {
        auto it = v.begin();

        oss << *it;

        while (++it != v.end()) {
            oss << sep << *it;
        }
    }

    return oss.str();
}

template <typename Tuple, std::size_t... Is>
void
join_tuple_impl(
    std::ostringstream& oss,
    const std::string& sep,
    const Tuple& t,
    std::index_sequence<Is...>
)
{
    // (from http://stackoverflow.com/a/6245777/273767)
    using swallow = int[]; // guarantees left to right order

    (void) swallow{
        0,
        (
            void(
                oss << (Is == 0 ? "" : sep) << std::get<Is>(t)
            ),
            0
        )...
    };
}

template <typename... Args>
void
join_tuple(
    std::ostringstream& oss,
    const std::string& sep,
    const std::tuple<Args...>& t
)
{ join_tuple_impl(oss, sep, t, std::index_sequence_for<Args...>{}); }

template <
    typename... Args,
    typename Enable = std::enable_if_t<(sizeof...(Args) > 1)>
>
std::string
join(const std::string& sep, Args&&... args)
{
    std::ostringstream oss;

    join_tuple(oss, sep, std::make_tuple(std::forward<Args>(args)...));

    return oss.str();
}

inline
std::string
indent(const std::string& what, const std::string& pad = "    ")
{
    std::string sep = "\n";
    std::string s;

    if (what.size() > 0) {
        s = pad + join(sep + pad, split(sep, what));
    }

    return s;
}

template <typename... Args>
std::string
cat_dir(Args&&... args)
{ return join("/", std::forward<Args>(args)...); }

template <typename... Args>
std::string
cat_file(Args&&... args)
{ return cat_dir(std::forward<Args>(args)...); }

template <typename T>
inline
T to_num(const std::string& s)
{
    return ((T) strtoull(s.c_str(), NULL, 10));
}

//
inline
__attribute__ ((const))
std::size_t
nearest_power_of_2(std::size_t x)
{
    return 1 << (64 - __builtin_clzl(x - 1));
}

// Number of bits used by v
template <typename T>
inline
__attribute__ ((const))
uint32_t
used_bits(const T v)
{
    if (v == 0) {
        return 0;
    }

    T index;

    __asm__("bsr %1, %0;" :"=r"(index) :"r"(v));

    return index + 1;
}

constexpr
std::size_t
bits_for_value(std::size_t v)
{ return v ? (v & 1) + bits_for_value(v >> 1) : 0; }

template <>
inline
__attribute__ ((const))
uint32_t
used_bits<int8_t>(const int8_t v)
{ return used_bits((int16_t) v); }

template <>
inline
__attribute__ ((const))
uint32_t
used_bits<uint8_t>(const uint8_t v)
{ return used_bits((uint16_t) v); }

inline
uint32_t
byte_size_of(uint32_t bits, uint32_t count)
{
    uint32_t bit_size = bits * count;

    return (bit_size / 8) + ((bit_size % 8) != 0 ? 1 : 0);
}

inline
uint32_t
byte_size_of(uint32_t bits)
{
    return byte_size_of(bits, 1);
}

template <
    typename T,
    typename = typename std::enable_if<
        std::is_arithmetic<T>::value
        &&
        !std::is_same<T, char>::value
    >::type
>
inline
auto
as_printable(T i) -> decltype(+i)
{ return +i; }

inline
auto
as_printable(char c)
{ return c; }

template <typename T>
inline
const T&
as_printable(
    const T& s,
    typename std::enable_if<
        !std::is_arithmetic<T>::value
    >::type* = 0
)
{ return s; }

template <typename T>
constexpr
typename std::underlying_type<T>::type
integral(T value)
{
    return static_cast<typename std::underlying_type<T>::type>(value);
}

template <typename T>
inline
std::size_t
digit_count(T v)
{
    return
        v == 0
        ? 1
        : ((std::size_t) std::log10(v)) + 1
        ;
}

template <typename T>
std::string
format_bits(T v)
{
    std::size_t nbits = sizeof(v) * 8;

    std::ostringstream oss;

    T mask = T{1} << (nbits - 1);

    for (std::size_t i = 0; i < nbits; ++i) {
        if (i % 4 == 0 && i != 0) {
            oss << " ";
        }

        oss << (v & mask ? '1' : '.');
        mask >>= 1;
    }

    return oss.str();
}

template <typename T>
std::string
format_bits_vertical(T v)
{
    std::size_t nbits = sizeof(v) * 8;

    std::ostringstream oss;

    T mask = 1;

    for (std::size_t i = 0; i < nbits; ++i) {
        oss << (v & mask ? '1' : '.') << "\n";
        mask <<= 1;
    }

    return oss.str();
}

} // namespace nx
