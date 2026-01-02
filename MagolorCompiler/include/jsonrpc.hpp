#pragma once
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

class JsonValue {
public:
  enum Type { Null, Bool, Int, Float, String, Array, Object };

  JsonValue() : type_(Null) {}
  JsonValue(bool v) : type_(Bool), boolVal_(v) {}
  JsonValue(int v) : type_(Int), intVal_(v) {}
  JsonValue(double v) : type_(Float), floatVal_(v) {}
  JsonValue(const std::string &v) : type_(String), strVal_(v) {}
  JsonValue(const char *v) : type_(String), strVal_(v) {}

  Type type() const { return type_; }
  bool isNull() const { return type_ == Null; }

  bool asBool() const { return boolVal_; }
  int asInt() const { return intVal_; }
  double asFloat() const { return floatVal_; }
  const std::string &asString() const { return strVal_; }

  std::vector<JsonValue> &asArray() { return arrVal_; }
  const std::vector<JsonValue> &asArray() const { return arrVal_; }

  std::unordered_map<std::string, JsonValue> &asObject() { return objVal_; }
  const std::unordered_map<std::string, JsonValue> &asObject() const {
    return objVal_;
  }

  JsonValue &operator[](const std::string &key) {
    if (type_ != Object) {
      type_ = Object;
      objVal_.clear();
    }
    return objVal_[key];
  }

  const JsonValue &operator[](const std::string &key) const {
    static JsonValue null;
    if (type_ != Object)
      return null;
    auto it = objVal_.find(key);
    return it != objVal_.end() ? it->second : null;
  }

  bool has(const std::string &key) const {
    return type_ == Object && objVal_.find(key) != objVal_.end();
  }

  void push(const JsonValue &v) {
    if (type_ != Array) {
      type_ = Array;
      arrVal_.clear();
    }
    arrVal_.push_back(v);
  }

  static JsonValue object() {
    JsonValue v;
    v.type_ = Object;
    return v;
  }
  static JsonValue array() {
    JsonValue v;
    v.type_ = Array;
    return v;
  }

  std::string serialize() const {
    std::ostringstream ss;
    serializeTo(ss);
    return ss.str();
  }

private:
  Type type_;
  bool boolVal_ = false;
  int intVal_ = 0;
  double floatVal_ = 0;
  std::string strVal_;
  std::vector<JsonValue> arrVal_;
  std::unordered_map<std::string, JsonValue> objVal_;

  void serializeTo(std::ostream &os) const {
    switch (type_) {
    case Null:
      os << "null";
      break;
    case Bool:
      os << (boolVal_ ? "true" : "false");
      break;
    case Int:
      os << intVal_;
      break;
    case Float:
      os << floatVal_;
      break;
    case String:
      os << '"';
      for (char c : strVal_) {
        if (c == '"')
          os << "\\\"";
        else if (c == '\\')
          os << "\\\\";
        else if (c == '\n')
          os << "\\n";
        else if (c == '\r')
          os << "\\r";
        else if (c == '\t')
          os << "\\t";
        else
          os << c;
      }
      os << '"';
      break;
    case Array:
      os << '[';
      for (size_t i = 0; i < arrVal_.size(); i++) {
        if (i > 0)
          os << ',';
        arrVal_[i].serializeTo(os);
      }
      os << ']';
      break;
    case Object:
      os << '{';
      bool first = true;
      for (auto &[k, v] : objVal_) {
        if (!first)
          os << ',';
        first = false;
        os << '"' << k << "\":";
        v.serializeTo(os);
      }
      os << '}';
      break;
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
    pos++; // skip closing "
    return JsonValue(result);
  }

  static JsonValue parseNumber(const std::string &s, size_t &pos) {
    size_t start = pos;
    bool isFloat = false;
    if (s[pos] == '-')
      pos++;
    while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9')
      pos++;
    if (pos < s.size() && s[pos] == '.') {
      isFloat = true;
      pos++;
    }
    while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9')
      pos++;
    std::string num = s.substr(start, pos - start);
    if (isFloat)
      return JsonValue(std::stod(num));
    return JsonValue(std::stoi(num));
  }

  static JsonValue parseArray(const std::string &s, size_t &pos) {
    pos++; // skip [
    JsonValue arr = JsonValue::array();
    skipWhitespace(s, pos);
    if (s[pos] == ']') {
      pos++;
      return arr;
    }
    while (true) {
      arr.push(parseValue(s, pos));
      skipWhitespace(s, pos);
      if (s[pos] == ']') {
        pos++;
        return arr;
      }
      if (s[pos] == ',')
        pos++;
    }
  }

  static JsonValue parseObject(const std::string &s, size_t &pos) {
    pos++; // skip {
    JsonValue obj = JsonValue::object();
    skipWhitespace(s, pos);
    if (s[pos] == '}') {
      pos++;
      return obj;
    }
    while (true) {
      skipWhitespace(s, pos);
      std::string key = parseString(s, pos).asString();
      skipWhitespace(s, pos);
      pos++; // skip :
      obj[key] = parseValue(s, pos);
      skipWhitespace(s, pos);
      if (s[pos] == '}') {
        pos++;
        return obj;
      }
      if (s[pos] == ',')
        pos++;
    }
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
    std::string header;
    int contentLength = -1;

    // Read headers line by line
    while (true) {
      if (!std::getline(std::cin, header)) {
        // EOF or error
        return std::nullopt;
      }

      // Remove trailing \r if present
      if (!header.empty() && header.back() == '\r') {
        header.pop_back();
      }

      // Empty line means end of headers
      if (header.empty()) {
        break;
      }

      // Parse Content-Length header
      if (header.find("Content-Length:") == 0) {
        try {
          contentLength = std::stoi(header.substr(15));
        } catch (...) {
          return std::nullopt;
        }
      }
    }

    if (contentLength < 0) {
      return std::nullopt;
    }

    // Read the JSON body
    std::string body(contentLength, '\0');
    if (!std::cin.read(&body[0], contentLength)) {
      return std::nullopt;
    }

    // Parse JSON
    JsonValue json = JsonParser::parse(body);
    Message msg;

    if (json.has("id")) {
      if (json["id"].type() == JsonValue::Int) {
        msg.id = json["id"].asInt();
      }
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
