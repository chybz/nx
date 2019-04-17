#include <unistd.h>
#include <cxxabi.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <fcntl.h>

#if defined(LINUX)
#include <sys/sendfile.h>
#elif defined(DARWIN)
#include <sys/socket.h>
#include <sys/uio.h>
#endif


#include <ios>
#include <iomanip>
#include <iostream>
#include <cstdio>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <regex>
#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <nx/utils.hpp>

namespace nx {

const std::size_t progress_ratio_width = 3;
const std::size_t progress_bar_chars = 4;
const std::size_t progress_bar_extra_width =
    progress_ratio_width + progress_bar_chars;
const std::size_t kilobytes = 1024;
const std::size_t megabytes = kilobytes * 1024;
const std::size_t gigabytes = megabytes * 1024;
const std::size_t terabytes = gigabytes * 1024;
const std::size_t petabytes = terabytes * 1024;

struct human_size_conv {
    const char* unit;
    std::size_t scale;
};

bool
interactive()
{
    return (
        ::isatty(::fileno(::stdin)) == 1
        &&
        ::isatty(::fileno(::stdout)) == 1
        &&
        (std::getenv("HARNESS_ACTIVE") == nullptr)
    );
}

std::string
lc(const std::string& s)
{
    std::string t = s;
    std::transform(s.begin(), s.end(), t.begin(), ::tolower);

    return t;
}

bool
debugged()
{
//     static bool checked = false;
//     static bool under_debugger = false;

// #if defined(LINUX)
//     auto trace_me = PTRACE_TRACEME;
//     auto trace_detach = PTRACE_DETACH;
// #elif defined(DARWIN)
//     auto trace_me = PT_TRACE_ME;
//     auto trace_detach = PT_DETACH;
// #endif

//     if (!checked) {
//         if (ptrace(trace_me, 0, 0, 0) < 0) {
//             under_debugger = true;
//         } else {
//             ptrace(trace_detach, 0, 0, 0);
//         }

//         checked = true;
//     }

//     return under_debugger;

    return false;
}

void set_progress(std::size_t x, std::size_t n, std::size_t w)
{
    if (!interactive()) {
        return;
    }

    float ratio = x / (float) n;
    std::size_t c = ratio * w;

    std::cout << "\r" << std::flush;

    std::cout << std::setw(3) << (std::size_t) (ratio * 100) << "% [";

    for (std::size_t x = 0; x < c; x++) {
        std::cout << "=";
    }

    for (std::size_t x = c; x < w; x++) {
        std::cout << " ";
    }

    std::cout << "]" << std::flush;
}

void clear_progress(std::size_t w)
{
    if (!interactive()) {
        return;
    }

    for (std::size_t x = 0; x < w + progress_bar_extra_width; x++) {
        std::cout << " ";
    }

    std::cout << "\r" << std::flush;
}

std::string
home_directory()
{
    const char* home = nullptr;
    std::string home_dir;

    home = getenv("HOME");

    if (home != nullptr) {
        home_dir = home;
    }

    if (home_dir.size() == 0) {
        throw std::runtime_error("failed to find home directory");
    }

    return home_dir;
}

bool
exists(const std::string& path)
{
    namespace fs = boost::filesystem;
    fs::path ppath(path);

    return fs::exists(ppath);
}

bool
directory_exists(const std::string& path)
{
    namespace fs = boost::filesystem;
    fs::path ppath(path);

    return (
        fs::exists(ppath)
        &&
        fs::is_directory(ppath)
    );
}

bool
file_exists(const std::string& path)
{
    namespace fs = boost::filesystem;
    fs::path ppath(path);

    return (
        fs::exists(ppath)
        &&
        fs::is_regular_file(ppath)
    );
}

bool
touch_file(const std::string& path)
{
    if (!mkfilepath(path)) {
        return false;
    }

    std::filebuf fbuf;

    fbuf.open(
        path,
        std::ios_base::in
        | std::ios_base::out
        | std::ios_base::app
        | std::ios_base::binary
    );

    if (!fbuf.is_open()) {
        return false;
    }

    fbuf.close();

    return true;
}

bool
truncate_file(const std::string& path, std::size_t size)
{
    if (!touch_file(path)) {
        return false;
    }

    if (truncate(path.c_str(), size) != 0) {
        return false;
    }

    return true;
}

bool
copy_file(const std::string& from, const std::string& to)
{
    namespace fs = boost::filesystem;

    if (!file_exists(from)) {
        return false;
    }

    std::string to_;

    if (directory_exists(to)) {
        // New file in existing directory
        to_ = fullpath(to, basename(from));
    } else {
        to_ = to;
    }

#if defined(LINUX)
    int src = ::open(from.c_str(), O_RDONLY, 0);

    if (src == -1) {
        throw std::runtime_error("failed to open file: " + from);
    }

    int dst = ::creat(to_.c_str(), 0644);

    if (dst == -1) {
        ::close(src);

        throw std::runtime_error("failed to create file: " + to);
    }

    auto ret = sendfile(dst, src, nullptr, file_size(from));

    ::close(src);
    ::close(dst);

    if (ret == -1) {
        throw std::runtime_error("failed to send file: " + from);
    }
#elif defined(DARWIN)
    // Sadly, sendfile doesn't support destination being a file
    std::ifstream source(from, std::ios::binary);
    std::ofstream dest(to_, std::ios::binary | std::ios::trunc);

    if (!(source.is_open() && dest.is_open())) {
        return false;
    }

    dest << source.rdbuf();

    source.close();
    dest.close();
#endif

    return true;
}

bool
write_file(const std::string& file, const char* buffer, std::size_t size)
{
    if (!mkfilepath(file)) {
        return false;
    }

    std::ofstream ofs(file, std::ios::binary | std::ios::trunc);

    if (!ofs.is_open()) {
        return false;
    }

    ofs.write(buffer, size);
    ofs.close();

    return true;
}

uint64_t
file_size(const std::string& path)
{
    namespace fs = boost::filesystem;
    fs::path ppath(path);

    return fs::file_size(ppath);
}

uint64_t
file_size_if_exists(const std::string& path)
{
    return
        file_exists(path)
        ? file_size(path)
        : 0
        ;
}

std::string
slurp(const std::string& path)
{
    std::string bytes;

    if (file_exists(path)) {
        std::ifstream ifs(path, std::ios::binary);

        if (ifs.is_open()) {
            auto fsize = file_size(path);

            bytes.resize(fsize);

            ifs.read(bytes.data(), fsize);
            ifs.close();
        }
    }

    return bytes;
}

bool
mkpath(const std::string& path)
{
    namespace fs = boost::filesystem;
    fs::path ppath(path);

    if (!fs::exists(ppath)) {
        return fs::create_directories(ppath);
    }

    return true;
}

unsigned long
rmpath(const std::string& path)
{
    if (!directory_exists(path)) {
        return 0;
    }

    namespace fs = boost::filesystem;
    fs::path ppath(path);

    return fs::remove_all(ppath);
}

bool
rmfile(const std::string& path)
{
    if (!file_exists(path)) {
        return false;
    }

    auto ret = unlink(path.c_str());

    return ret == 0;
}

bool
mkfilepath(const std::string& path)
{
    bool ret = true;
    namespace fs = boost::filesystem;
    fs::path ppath(path);

    ppath.normalize();

    if (!ppath.has_filename()) {
        throw std::runtime_error("path has no filename");
    }

    fs::path parent = ppath.parent_path();
    fs::path filename = ppath.filename();

    if (!fs::exists(parent)) {
        ret = fs::create_directories(parent);
    }

    return ret;
}

void
rename(const std::string& from_path, const std::string& to_path)
{
    namespace fs = boost::filesystem;
    fs::path from_ppath(from_path);
    fs::path to_ppath(to_path);

    fs::rename(from_ppath, to_ppath);
}

std::string
fullpath(const std::string& rel)
{
    namespace fs = boost::filesystem;
    fs::path ppath(rel);

    return fs::complete(ppath).string();
}

std::string
fullpath(const std::string& dir, const std::string& file)
{
    namespace fs = boost::filesystem;
    fs::path ppath(dir);

    ppath.normalize();
    ppath /= file;

    return ppath.normalize().string();
}

std::string
basename(const std::string& path, const std::string& ext)
{
    namespace fs = boost::filesystem;
    fs::path ppath(path);

    ppath.normalize();

    if (ext.empty()) {
        return ppath.filename().string();
    }

    auto bn = ppath.filename().string();
    auto pos = bn.rfind(ext);

    if (
        pos != std::string::npos
        &&
        pos + ext.size() == bn.size()
    ) {
        bn.erase(pos);
    }

    return bn;
}

std::string
dirname(const std::string& path)
{
    namespace fs = boost::filesystem;
    fs::path ppath(path);

    ppath.normalize();

    return ppath.parent_path().string();
}

unsigned long long
human_readable_size(const std::string& expr)
{
    static std::vector<human_size_conv> ct = {
        { "K", kilobytes },
        { "M", megabytes },
        { "G", gigabytes },
        { "T", terabytes },
        { "P", petabytes }
    };

    std::regex sizeexpr("(\\d+)(?:(K|M|G|T|P))?");
    std::smatch what;
    unsigned long long scale = 1;
    unsigned long long size = 0;

    if (regex_match(expr, what, sizeexpr)) {
        size = to_num<unsigned long long>(what[1]);

        for (const auto& c : ct) {
            if (what[2] == c.unit) {
                scale = c.scale;
                break;
            }
        }

        size *= scale;
    } else {
        throw std::runtime_error("invalid size: " + expr);
    }

    return size;
}

std::string
human_readable_size(uint64_t size, const std::string& user_unit)
{
    static std::vector<human_size_conv> ct = {
        { "P", petabytes },
        { "T", terabytes },
        { "G", gigabytes },
        { "M", megabytes },
        { "K", kilobytes }
    };

    std::ostringstream oss;
    double value = size;
    const char* unit = "";
    std::size_t scale = 1;

    for (const auto& c : ct) {
        if (size >= c.scale) {
            scale = c.scale;
            unit = c.unit;
            break;
        }
    }

    value /= scale;

    char buf[50 + 1];

    std::snprintf(buf, sizeof(buf), "%.2f", value);

    oss << buf << unit << user_unit;

    return oss.str();
}

std::string
human_readable_file_size(const std::string& file)
{ return human_readable_size(file_size(file)); }

std::string
human_readable_rate(
    uint64_t count,
    uint64_t millisecs,
    const std::string& user_unit
)
{
    double secs = millisecs / 1000.0;

    double count_per_sec = count;
    count_per_sec /= secs;

    std::ostringstream oss;

    oss
        << human_readable_size(
            std::llround(count_per_sec),
            user_unit
        )
        << "/s"
        ;

    return oss.str();
}

std::string
plural(
    const std::string& base,
    const std::string& p,
    std::size_t count
)
{
    return
        count > 1
        ? base + p
        : base
        ;
}

bool
has_spaces(const std::string& s)
{
    bool has_spaces = std::any_of(
        s.begin(), s.end(),
        [](auto c) { return std::isspace(c); }
    );

    return has_spaces;
}

bool
has_spaces(const char* s)
{ return has_spaces(std::string(s)); }

void
split(const std::string& re, const std::string& expr, strings& list)
{
    list.clear();

    if (expr.empty()) {
        return;
    }

    std::regex delim(re);

    auto cur = std::sregex_token_iterator(
        expr.begin(), expr.end(),
        delim,
        -1
    );
    auto end = std::sregex_token_iterator();

    for( ; cur != end; ++cur ) {
        list.push_back(*cur);
    }

    if (list.empty() && expr.size() > 0) {
        list.push_back(expr);
    }
}

bool
match(const std::string& expr, const std::string& re)
{
    std::regex mre(re);

    return regex_match(expr, mre);
}

bool
match(const std::string& expr, const std::string& re, std::smatch& m)
{
    std::regex mre(re);

    return regex_match(expr, m, mre);
}

std::string
subst(const std::string& s, const std::string& re, const std::string& what)
{
    return std::regex_replace(
        s,
        std::regex(re),
        what
    );
}

std::string
clean_path(const std::string& path)
{
    // Note: simpler char-based algorithms will not work with UTF8 data...
    std::string cleaned;
    auto parts = split("/", path);

    for (const auto& part : parts) {
        if (part.empty()) {
            continue;
        }

        cleaned += '/';
        cleaned += part;
    }

    return cleaned;
}

void
chomp(std::string& s)
{
    std::string::size_type pos = s.find_last_not_of("\r\n");

    if (pos != std::string::npos) {
        s = s.substr(0, pos + 1);
    } else if (s.size() > 0) {
        s.clear();
    }
}

std::string
quote(const std::string& s)
{
    std::string quoted;

    if (!s.empty()) {
        if (s.find(" ") != std::string::npos) {
            quoted = "\"" + s + "\"";
        } else {
            quoted = s;
        }
    }

    return quoted;
}

void
unquote(std::string& s)
{
    boost::algorithm::trim_if(s, boost::algorithm::is_any_of("\""));
}

strings
glob(const std::string& expr)
{
    glob_t g;
    strings patterns = split("\\s+", expr);
    strings matches;
    int flags = 0;

    if (!patterns.empty()) {
        for (const auto& pattern : patterns) {
            glob(pattern.c_str(), flags, NULL, &g);

            // Calling with GLOB_APPEND the first time segfaults...
            // (glob() surely tries to free unallocated data)
            flags |= GLOB_APPEND;
        }

        if (g.gl_pathc > 0) {
            for (size_t i = 0; i < g.gl_pathc; i++) {
                matches.push_back(g.gl_pathv[i]);
            }
        }

        globfree(&g);
    }

    return matches;
}

bool
extract_delimited(
    std::string& from,
    std::string& to,
    unsigned char delim,
    unsigned char quote
)
{
    to.clear();

    // Find the first semicolon not surrounded by the quote char
    // (if specified)
    unsigned int quote_count = 0;
    bool found = false;
    std::string::iterator iter;

    for (iter = from.begin(); iter != from.end(); ++iter) {
        if (quote != '\0' && *iter == quote) {
            quote_count++;
        } else if (*iter == delim && (quote_count % 2 == 0)) {
            // Found it
            found = true;
            break;
        }
    }

    if (found) {
        to.assign(from.begin(), iter);

        // Remove to from input
        from.erase(from.begin(), iter + 1);
    }

    return found;
}

std::string
demangle_type_name(const std::string& mangled)
{
    char* buffer;
    int status;

    buffer = abi::__cxa_demangle(mangled.c_str(), 0, 0, &status);

    if (status == 0) {
        std::string n(buffer);
        free(buffer);

        return n;
    } else {
        return std::string("demangle failure");
    }

    return std::string("unsupported");
}

} // namespace nx
