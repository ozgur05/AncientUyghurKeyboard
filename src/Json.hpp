// Json.hpp — self-contained, header-only JSON parser (no dependencies).
//
// Supports the full JSON grammar plus JSONC comments (// and /* */) so that
// layout files can be documented inline. Parsing only (no serialization) —
// that is all the layout loader needs. UTF-8 in, UTF-8 out; \uXXXX escapes
// (including surrogate pairs) are decoded to UTF-8.
#pragma once

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdint>
#include <utility>

namespace json {

enum class Type { Null, Bool, Number, String, Array, Object };

class Value;
using Array  = std::vector<Value>;
using Object = std::map<std::string, Value>;

// A JSON value. Storage is simple (all members present) rather than a tagged
// union — clarity over micro-optimization; layout files are tiny.
class Value {
public:
    Value()                 : m_type(Type::Null) {}
    Value(std::nullptr_t)   : m_type(Type::Null) {}
    Value(bool b)           : m_type(Type::Bool),   m_bool(b) {}
    Value(double d)         : m_type(Type::Number), m_num(d) {}
    Value(int i)            : m_type(Type::Number), m_num(static_cast<double>(i)) {}
    Value(const char* s)    : m_type(Type::String), m_str(s) {}
    Value(std::string s)    : m_type(Type::String), m_str(std::move(s)) {}
    Value(Array a)          : m_type(Type::Array),  m_arr(std::move(a)) {}
    Value(Object o)         : m_type(Type::Object), m_obj(std::move(o)) {}

    Type type()     const { return m_type; }
    bool isNull()   const { return m_type == Type::Null; }
    bool isBool()   const { return m_type == Type::Bool; }
    bool isNumber() const { return m_type == Type::Number; }
    bool isString() const { return m_type == Type::String; }
    bool isArray()  const { return m_type == Type::Array; }
    bool isObject() const { return m_type == Type::Object; }

    bool               asBool(bool d = false)   const { return isBool()   ? m_bool : d; }
    double             asNumber(double d = 0.0) const { return isNumber() ? m_num  : d; }
    int                asInt(int d = 0)         const { return isNumber() ? static_cast<int>(m_num) : d; }
    const std::string& asString()               const { return m_str; }
    const Array&       asArray()                 const { return m_arr; }
    const Object&      asObject()                const { return m_obj; }

    bool contains(const std::string& k) const {
        return m_type == Type::Object && m_obj.find(k) != m_obj.end();
    }
    // Safe accessor: returns a static Null value for missing keys / non-objects.
    const Value& operator[](const std::string& k) const {
        static const Value nul;
        if (m_type != Type::Object) return nul;
        auto it = m_obj.find(k);
        return it == m_obj.end() ? nul : it->second;
    }
    size_t size() const {
        if (m_type == Type::Array)  return m_arr.size();
        if (m_type == Type::Object) return m_obj.size();
        return 0;
    }

private:
    Type        m_type;
    bool        m_bool = false;
    double      m_num  = 0.0;
    std::string m_str;
    Array       m_arr;
    Object      m_obj;
};

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& m, size_t ln, size_t col)
        : std::runtime_error("JSON parse error at line " + std::to_string(ln) +
                             ", column " + std::to_string(col) + ": " + m),
          line(ln), column(col) {}
    size_t line;
    size_t column;
};

namespace detail {

inline void appendUtf8(std::string& s, uint32_t cp)
{
    if (cp <= 0x7F) {
        s += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        s += static_cast<char>(0xC0 | (cp >> 6));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        s += static_cast<char>(0xE0 | (cp >> 12));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        s += static_cast<char>(0xF0 | (cp >> 18));
        s += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

class Parser {
public:
    explicit Parser(const std::string& s) : m_s(s) {}

    Value parse() {
        skipWs();
        Value v = parseValue();
        skipWs();
        if (m_pos != m_s.size())
            fail("trailing characters after top-level value");
        return v;
    }

private:
    const std::string& m_s;
    size_t m_pos  = 0;
    size_t m_line = 1;
    size_t m_col  = 1;

    [[noreturn]] void fail(const std::string& msg) { throw ParseError(msg, m_line, m_col); }

    bool eof()  const { return m_pos >= m_s.size(); }
    char peek() const { return eof() ? '\0' : m_s[m_pos]; }

    char get() {
        char c = m_s[m_pos++];
        if (c == '\n') { ++m_line; m_col = 1; } else { ++m_col; }
        return c;
    }

    void skipWs() {
        while (!eof()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') get();
            else if (c == '/') skipComment();
            else break;
        }
    }

    void skipComment() {
        get();                              // consume first '/'
        if (peek() == '/') {                // line comment
            while (!eof() && peek() != '\n') get();
        } else if (peek() == '*') {         // block comment
            get();
            while (!eof()) {
                if (peek() == '*') { get(); if (peek() == '/') { get(); return; } }
                else get();
            }
            fail("unterminated block comment");
        } else {
            fail("unexpected '/'");
        }
    }

    Value parseValue() {
        skipWs();
        if (eof()) fail("unexpected end of input");
        char c = peek();
        switch (c) {
            case '{': return parseObject();
            case '[': return parseArray();
            case '"': return Value(parseString());
            case 't':
            case 'f': return parseBool();
            case 'n': return parseNull();
            default:
                if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
                fail(std::string("unexpected character '") + c + "'");
        }
        return Value(); // unreachable
    }

    Value parseObject() {
        get(); // '{'
        Object obj;
        skipWs();
        if (peek() == '}') { get(); return Value(std::move(obj)); }
        while (true) {
            skipWs();
            if (peek() != '"') fail("expected string key in object");
            std::string key = parseString();
            skipWs();
            if (get() != ':') fail("expected ':' after object key");
            Value v = parseValue();
            obj[std::move(key)] = std::move(v);
            skipWs();
            char c = get();
            if (c == ',') continue;
            if (c == '}') break;
            fail("expected ',' or '}' in object");
        }
        return Value(std::move(obj));
    }

    Value parseArray() {
        get(); // '['
        Array arr;
        skipWs();
        if (peek() == ']') { get(); return Value(std::move(arr)); }
        while (true) {
            arr.push_back(parseValue());
            skipWs();
            char c = get();
            if (c == ',') continue;
            if (c == ']') break;
            fail("expected ',' or ']' in array");
        }
        return Value(std::move(arr));
    }

    std::string parseString() {
        if (get() != '"') fail("expected '\"'");
        std::string out;
        while (true) {
            if (eof()) fail("unterminated string");
            char c = get();
            if (c == '"') break;
            if (c == '\\') {
                if (eof()) fail("unterminated escape sequence");
                char e = get();
                switch (e) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    case 'u': {
                        uint32_t cp = parseHex4();
                        if (cp >= 0xD800 && cp <= 0xDBFF) {         // high surrogate
                            if (get() != '\\' || get() != 'u')
                                fail("expected '\\u' low surrogate");
                            uint32_t lo = parseHex4();
                            if (lo < 0xDC00 || lo > 0xDFFF) fail("invalid low surrogate");
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                            fail("unexpected low surrogate");
                        }
                        appendUtf8(out, cp);
                        break;
                    }
                    default: fail("invalid escape character");
                }
            } else {
                out += c;
            }
        }
        return out;
    }

    uint32_t parseHex4() {
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            if (eof()) fail("incomplete \\u escape");
            char c = get();
            v <<= 4;
            if      (c >= '0' && c <= '9') v |= static_cast<uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') v |= static_cast<uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= static_cast<uint32_t>(c - 'A' + 10);
            else fail("invalid hex digit in \\u escape");
        }
        return v;
    }

    Value parseNumber() {
        size_t start = m_pos;
        if (peek() == '-') get();
        while (peek() >= '0' && peek() <= '9') get();
        if (peek() == '.') { get(); while (peek() >= '0' && peek() <= '9') get(); }
        if (peek() == 'e' || peek() == 'E') {
            get();
            if (peek() == '+' || peek() == '-') get();
            while (peek() >= '0' && peek() <= '9') get();
        }
        std::string num = m_s.substr(start, m_pos - start);
        try {
            return Value(std::stod(num));
        } catch (...) {
            fail("invalid number literal");
        }
        return Value(); // unreachable
    }

    Value parseBool() {
        if (m_s.compare(m_pos, 4, "true") == 0)  { m_pos += 4; m_col += 4; return Value(true); }
        if (m_s.compare(m_pos, 5, "false") == 0) { m_pos += 5; m_col += 5; return Value(false); }
        fail("invalid literal");
        return Value(); // unreachable
    }

    Value parseNull() {
        if (m_s.compare(m_pos, 4, "null") == 0) { m_pos += 4; m_col += 4; return Value(nullptr); }
        fail("invalid literal");
        return Value(); // unreachable
    }
};

} // namespace detail

// Parse UTF-8 JSON text. Throws ParseError on malformed input.
inline Value parse(const std::string& text) {
    detail::Parser p(text);
    return p.parse();
}

} // namespace json
