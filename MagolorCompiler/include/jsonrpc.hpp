#pragma once
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cctype>

#include <variant>

class JsonValue {
public:
  enum Type { Null, Bool, Int, Float, String, Array, Object };

  using ObjectMap = std::unordered_map<std::string, JsonValue>;
  using ArrayVec = std::vector<JsonValue>;

private:
  std::variant<std::nullptr_t, bool, int, double, std::string, ArrayVec, ObjectMap> data_;

public:
  JsonValue() : data_(nullptr) {}
  JsonValue(bool v) : data_(v) {}
  JsonValue(int v) : data_(v) {}
  JsonValue(double v) : data_(v) {}
  JsonValue(const std::string &v) : data_(v) {}
  JsonValue(const char *v) : data_(std::string(v)) {}

  Type type() const { return static_cast<Type>(data_.index()); }
  bool isNull() const { return std::holds_alternative<std::nullptr_t>(data_); }

  bool asBool() const { return std::get<bool>(data_); }
  int asInt() const { return std::get<int>(data_); }
  double asFloat() const { return std::get<double>(data_); }
  const std::string &asString() const { return std::get<std::string>(data_); }

  ArrayVec &asArray() { return std::get<ArrayVec>(data_); }
  const ArrayVec &asArray() const { return std::get<ArrayVec>(data_); }

  ObjectMap &asObject() { return std::get<ObjectMap>(data_); }
  const ObjectMap &asObject() const { return std::get<ObjectMap>(data_); }

  JsonValue &operator[](const std::string &key) {
    if (!std::holds_alternative<ObjectMap>(data_)) {
      data_ = ObjectMap{};
    }
    return std::get<ObjectMap>(data_)[key];
  }

  const JsonValue &operator[](const std::string &key) const {
    static JsonValue null;
    if (!std::holds_alternative<ObjectMap>(data_)) return null;
    auto &obj = std::get<ObjectMap>(data_);
    auto it = obj.find(key);
    return it != obj.end() ? it->second : null;
  }

  bool has(const std::string &key) const {
    if (!std::holds_alternative<ObjectMap>(data_)) return false;
    return std::get<ObjectMap>(data_).find(key) != std::get<ObjectMap>(data_).end();
  }

  void push(const JsonValue &v) {
    if (!std::holds_alternative<ArrayVec>(data_)) {
      data_ = ArrayVec{};
    }
    std::get<ArrayVec>(data_).push_back(v);
  }

  static JsonValue object() {
    JsonValue v;
    v.data_ = ObjectMap{};
    return v;
  }

  static JsonValue array() {
    JsonValue v;
    v.data_ = ArrayVec{};
    return v;
  }

  std::string serialize() const {
    std::ostringstream ss;
    serializeTo(ss);
    return ss.str();
  }

private:
  void serializeTo(std::ostream &os) const {
    switch (data_.index()) {
    case 0: os << "null"; break;
    case 1: os << (std::get<bool>(data_) ? "true" : "false"); break;
    case 2: os << std::get<int>(data_); break;
    case 3: os << std::get<double>(data_); break;
    case 4:
      os << '"';
      for (char c : std::get<std::string>(data_)) {
        if (c == '"') os << "\\\"";
        else if (c == '\\') os << "\\\\";
        else if (c == '\n') os << "\\n";
        else if (c == '\r') os << "\\r";
        else if (c == '\t') os << "\\t";
        else os << c;
      }
      os << '"';
      break;
    case 5: {
      os << '[';
      const auto &arr = std::get<ArrayVec>(data_);
      for (size_t i = 0; i < arr.size(); i++) {
        if (i > 0) os << ',';
        arr[i].serializeTo(os);
      }
      os << ']';
      break;
    }
    case 6: {
      os << '{';
      bool first = true;
      for (const auto &[k, v] : std::get<ObjectMap>(data_)) {
        if (!first) os << ',';
        first = false;
        os << '"' << k << "\":";
        v.serializeTo(os);
      }
      os << '}';
      break;
    }
    }
  }
};

class JsonParser {
public:
  static JsonValue parse(const std::string &s) {
    size_t pos = 0;
    return parseValue(s, pos);
  }

private:
  static void skipWhitespace(const std::string &s, size_t &pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' ||
                              s[pos] == '\n' || s[pos] == '\r'))
      pos++;
  }

  static JsonValue parseValue(const std::string &s, size_t &pos) {
    skipWhitespace(s, pos);
    if (pos >= s.size())
      return JsonValue();

    char c = s[pos];
    if (c == 'n')
      return parseNull(s, pos);
    if (c == 't' || c == 'f')
      return parseBool(s, pos);
    if (c == '"')
      return parseString(s, pos);
    if (c == '[')
      return parseArray(s, pos);
    if (c == '{')
      return parseObject(s, pos);
    if (c == '-' || (c >= '0' && c <= '9'))
      return parseNumber(s, pos);
    return JsonValue();
  }

  static JsonValue parseNull(const std::string &s, size_t &pos) {
    pos += 4;
    return JsonValue();
  }

  static JsonValue parseBool(const std::string &s, size_t &pos) {
    if (s[pos] == 't') {
      pos += 4;
      return JsonValue(true);
    }
    pos += 5;
    return JsonValue(false);
  }

  static JsonValue parseString(const std::string &s, size_t &pos) {
    pos++; // skip "
    std::string result;
    while (pos < s.size() && s[pos] != '"') {
      if (s[pos] == '\\' && pos + 1 < s.size()) {
        pos++;
        switch (s[pos]) {
        case 'n':
          result += '\n';
          break;
        case 'r':
          result += '\r';
          break;
        case 't':
          result += '\t';
          break;
        case '"':
          result += '"';
          break;
        case '\\':
          result += '\\';
          break;
        default:
          result += s[pos];
        }
      } else {
        result += s[pos];
      }
      pos++;
    }
    if (pos < s.size()) pos++; // skip closing "
    return JsonValue(result);
  }

  static JsonValue parseNumber(const std::string &s, size_t &pos) {
    size_t start = pos;
    bool isFloat = false;
    bool negative = false;
    
    if (pos < s.size() && s[pos] == '-') {
      negative = true;
      pos++;
    }
    
    while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9')
      pos++;
      
    if (pos < s.size() && s[pos] == '.') {
      isFloat = true;
      pos++;
      while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9')
        pos++;
    }
    
    if (pos == start || (negative && pos == start + 1)) {
      return JsonValue(0);
    }
    
    std::string num = s.substr(start, pos - start);
    
    if (isFloat) {
      // Manual float parse to avoid exceptions
      double result = 0;
      bool pastDot = false;
      double decimal = 0.1;
      size_t i = negative ? 1 : 0;
      for (; i < num.size(); i++) {
        if (num[i] == '.') {
          pastDot = true;
          continue;
        }
        if (pastDot) {
          result += (num[i] - '0') * decimal;
          decimal *= 0.1;
        } else {
          result = result * 10 + (num[i] - '0');
        }
      }
      return JsonValue(negative ? -result : result);
    }
    
    // Manual int parse to avoid exceptions
    int result = 0;
    size_t i = negative ? 1 : 0;
    for (; i < num.size(); i++) {
      result = result * 10 + (num[i] - '0');
    }
    return JsonValue(negative ? -result : result);
  }

  static JsonValue parseArray(const std::string &s, size_t &pos) {
    pos++; // skip [
    JsonValue arr = JsonValue::array();
    skipWhitespace(s, pos);
    if (pos < s.size() && s[pos] == ']') {
      pos++;
      return arr;
    }
    while (pos < s.size()) {
      arr.push(parseValue(s, pos));
      skipWhitespace(s, pos);
      if (pos >= s.size() || s[pos] == ']') {
        if (pos < s.size()) pos++;
        return arr;
      }
      if (s[pos] == ',')
        pos++;
    }
    return arr;
  }

  static JsonValue parseObject(const std::string &s, size_t &pos) {
    pos++; // skip {
    JsonValue obj = JsonValue::object();
    skipWhitespace(s, pos);
    if (pos < s.size() && s[pos] == '}') {
      pos++;
      return obj;
    }
    while (pos < s.size()) {
      skipWhitespace(s, pos);
      if (pos >= s.size() || s[pos] != '"') break;
      std::string key = parseString(s, pos).asString();
      skipWhitespace(s, pos);
      if (pos >= s.size()) break;
      if (s[pos] == ':') pos++; // skip :
      obj[key] = parseValue(s, pos);
      skipWhitespace(s, pos);
      if (pos >= s.size() || s[pos] == '}') {
        if (pos < s.size()) pos++;
        return obj;
      }
      if (s[pos] == ',')
        pos++;
    }
    return obj;
  }
};

struct Message {
  std::string jsonrpc = "2.0";
  std::optional<int> id;
  std::string method;
  JsonValue params;
  JsonValue result;
  JsonValue error;

  bool isRequest() const { return id.has_value() && !method.empty(); }
  bool isResponse() const { return id.has_value() && method.empty(); }
  bool isNotification() const { return !id.has_value() && !method.empty(); }
};

class Transport {
public:
  std::optional<Message> receive() {
    int contentLength = -1;
    
    // Read headers line by line
    while (true) {
      std::string line;
      
      // Read chars until newline
      while (true) {
        int ch = std::cin.get();
        
        if (ch == EOF || !std::cin.good()) {
          return std::nullopt;
        }
        if (ch == '\r') {
          continue;
        }
        if (ch == '\n') {
          break;
        }
        line += static_cast<char>(ch);
      }
      
      // Empty line = end of headers
      if (line.empty()) {
        break;
      }
      
      // Find colon
      size_t colonPos = line.find(':');
      if (colonPos == std::string::npos) {
        continue;
      }
      
      std::string name = line.substr(0, colonPos);
      std::string value = line.substr(colonPos + 1);
      
      // Lowercase name
      for (size_t i = 0; i < name.size(); i++) {
        if (name[i] >= 'A' && name[i] <= 'Z') {
          name[i] = name[i] - 'A' + 'a';
        }
      }
      
      // Trim value
      size_t start = 0;
      while (start < value.size() && (value[start] == ' ' || value[start] == '\t')) {
        start++;
      }
      size_t end = value.size();
      while (end > start && (value[end-1] == ' ' || value[end-1] == '\t')) {
        end--;
      }
      value = value.substr(start, end - start);
      
      if (name == "content-length" && !value.empty()) {
        // Manual int parse - NO exceptions
        contentLength = 0;
        bool valid = true;
        for (size_t i = 0; i < value.size(); i++) {
          char c = value[i];
          if (c >= '0' && c <= '9') {
            contentLength = contentLength * 10 + (c - '0');
          } else {
            valid = false;
            break;
          }
        }
        if (!valid) {
          contentLength = -1;
        }
      }
    }
    
    if (contentLength <= 0) {
      return std::nullopt;
    }
    
    // Read body
    std::string body(contentLength, '\0');
    std::cin.read(&body[0], contentLength);
    
    if (std::cin.gcount() != contentLength) {
      return std::nullopt;
    }
    
    // Parse JSON
    JsonValue json = JsonParser::parse(body);
    Message msg;
    
    if (json.has("id") && json["id"].type() == JsonValue::Int) {
      msg.id = json["id"].asInt();
    }
    if (json.has("method")) {
      msg.method = json["method"].asString();
    }
    if (json.has("params")) {
      msg.params = json["params"];
    }
    if (json.has("result")) {
      msg.result = json["result"];
    }
    if (json.has("error")) {
      msg.error = json["error"];
    }
    
    return msg;
  }

  void send(const Message &msg) {
    JsonValue json = JsonValue::object();
    json["jsonrpc"] = "2.0";
    if (msg.id.has_value()) {
      json["id"] = msg.id.value();
    }
    if (!msg.method.empty()) {
      json["method"] = msg.method;
    }
    if (!msg.params.isNull()) {
      json["params"] = msg.params;
    }
    if (!msg.result.isNull()) {
      json["result"] = msg.result;
    }
    if (!msg.error.isNull()) {
      json["error"] = msg.error;
    }

    std::string body = json.serialize();
    std::cout << "Content-Length: " << body.size() << "\r\n\r\n" << body;
    std::cout.flush();
  }

  void respond(int id, const JsonValue &result) {
    Message msg;
    msg.id = id;
    msg.result = result;
    send(msg);
  }

  void respondError(int id, int code, const std::string &message) {
    Message msg;
    msg.id = id;
    msg.error = JsonValue::object();
    msg.error["code"] = code;
    msg.error["message"] = message;
    send(msg);
  }

  void notify(const std::string &method, const JsonValue &params) {
    Message msg;
    msg.method = method;
    msg.params = params;
    send(msg);
  }
};
