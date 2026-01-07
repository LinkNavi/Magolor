#include "codegen.hpp"
#include "stdlib.hpp"
#include <unordered_set>
#include <variant>
#include <functional>

void CodeGen::emit(const std::string &s) { out << s; }
void CodeGen::emitLine(const std::string &s) {
  emitIndent();
  out << s << "\n";
}
void CodeGen::emitIndent() {
  for (int i = 0; i < indent; i++)
    out << "    ";
}

std::string CodeGen::typeToString(const TypePtr &type) {
  if (!type)
    return "auto";
  switch (type->kind) {
  case Type::INT:
    return "int";
  case Type::FLOAT:
    return "double";
  case Type::STRING:
    return "std::string";
  case Type::BOOL:
    return "bool";
  case Type::VOID:
    return "void";
  case Type::CLASS:
    return type->className;
  case Type::OPTION:
    return "std::optional<" + typeToString(type->innerType) + ">";
  case Type::ARRAY:
    return "std::vector<" + typeToString(type->innerType) + ">";
  case Type::GENERIC: {
    // Handle generic types like Map<K,V>
    std::string result = type->className;
    if (!type->genericArgs.empty()) {
      result += "<";
      for (size_t i = 0; i < type->genericArgs.size(); i++) {
        if (i > 0)
          result += ", ";
        result += typeToString(type->genericArgs[i]);
      }
      result += ">";
    }

    // Map Magolor generic types to C++ equivalents
    if (type->className == "Map" && type->genericArgs.size() == 2) {
      return "std::unordered_map<" + typeToString(type->genericArgs[0]) + ", " +
             typeToString(type->genericArgs[1]) + ">";
    }
    if (type->className == "Set" && type->genericArgs.size() == 1) {
      return "std::unordered_set<" + typeToString(type->genericArgs[0]) + ">";
    }

    return result;
  }
  case Type::FUNCTION: {
    std::string s = "std::function<" + typeToString(type->returnType) + "(";
    for (size_t i = 0; i < type->paramTypes.size(); i++) {
      if (i > 0)
        s += ", ";
      s += typeToString(type->paramTypes[i]);
    }
    return s + ")>";
  }
  }
  return "auto";
}

void CodeGen::genStdLib() { out << StdLibGenerator::generateAll(); }

void CodeGen::genCImports(const std::vector<CImportDecl> &cimports) {
  if (cimports.empty())
    return;

  out << "// "
         "==================================================================="
         "\n";
  out << "// C/C++ Imports\n";
  out << "// "
         "==================================================================="
         "\n";

  for (const auto &imp : cimports) {
    // Generate include directive
    if (imp.isSystemHeader) {
      out << "#include <" << imp.header << ">\n";
    } else {
      out << "#include \"" << imp.header << "\"\n";
    }

    // If namespace alias is provided, create it
    if (!imp.asNamespace.empty()) {
      out << "namespace " << imp.asNamespace << " {\n";

      if (!imp.symbols.empty()) {
        // Import specific symbols
        for (const auto &sym : imp.symbols) {
          out << "    using ::" << sym << ";\n";
        }
      } else {
        // No specific symbols - import common std:: functions
        out << "    // Common C++ standard library functions\n";
        out << "    using std::sqrt; using std::sin; using std::cos; using "
               "std::tan;\n";
        out << "    using std::asin; using std::acos; using std::atan; using "
               "std::atan2;\n";
        out << "    using std::pow; using std::exp; using std::log; using "
               "std::log10;\n";
        out << "    using std::abs; using std::fabs; using std::floor; using "
               "std::ceil;\n";
        out << "    using std::round; using std::fmod; using std::cbrt;\n";
      }

      out << "}\n";
      importedNamespaces.insert(imp.asNamespace);
    } else if (!imp.symbols.empty()) {
      // Import specific symbols into global namespace
      for (const auto &sym : imp.symbols) {
        out << "using ::" << sym << ";\n";
      }
    }
  }

  out << "\n";
}

bool CodeGen::isClassName(const std::string &name) const {
  return knownClassNames.count(name) > 0;
}



std::string CodeGen::generate(const Program &prog) {
  out.str("");
  out.clear();
  importedNamespaces.clear();
  knownClassNames.clear();
  
  // ============================================================================
  // STEP 1: Generate includes FIRST
  // ============================================================================
  out << "// Auto-generated C++ code from Magolor\n";
  
  // NEW: Output @cpp_header blocks BEFORE standard includes
  if (!prog.cppHeaders.empty()) {
    out << "// User-provided C++ headers\n";
    for (const auto &header : prog.cppHeaders) {
      out << header.code;
      if (!header.code.empty() && header.code.back() != '\n') {
        out << "\n";
      }
    }
    out << "\n";
  }
  
  // Standard includes
  out << "#include <vector>\n";
  out << "#include <unordered_map>\n"; 
  out << "#include <optional>\n";
  out << "#include <iostream>\n";
  out << "#include <string>\n";
  out << "#include <algorithm>\n";
  out << "#include <functional>\n";
  out << "#include <sstream>\n";
  out << "#include <fstream>\n";
  out << "#include <filesystem>\n";
  out << "#include <random>\n";
  out << "#include <chrono>\n";
  out << "#include <thread>\n";
  out << "#include <cmath>\n\n";
  
  // Collect all class names
  for (const auto &cls : prog.classes) {
    knownClassNames.insert(cls.name);
  }
  
  // Generate C/C++ imports
  genCImports(prog.cimports);  // ============================================================================
  // STEP 2: Generate stdlib helpers ONCE - using header guards
  // ============================================================================
  out << "// ============================================================================\n";
  out << "// Standard Library Helpers (generated once)\n";
  out << "// ============================================================================\n";
  out << "#ifndef MAGOLOR_STDLIB_HELPERS_H\n";
  out << "#define MAGOLOR_STDLIB_HELPERS_H\n\n";
  
  // String conversion helpers
  out << "// Template helpers for string conversion\n";
  out << "template<typename T>\n";
  out << "inline std::string mg_to_string(const T& val) { \n";
  out << "    std::ostringstream oss; \n";
  out << "    oss << val; \n";
  out << "    return oss.str(); \n";
  out << "}\n\n";
  
  out << "template<>\n";
  out << "inline std::string mg_to_string(const bool& val) {\n";
  out << "    return val ? \"true\" : \"false\";\n";
  out << "}\n\n";
  
  out << "template<>\n";
  out << "inline std::string mg_to_string(const std::string& val) {\n";
  out << "    return val;\n";
  out << "}\n\n";
  
  // Option helpers
  out << "// Global Option helpers\n";
  out << "template<typename T>\n";
  out << "inline bool isSome(const std::optional<T>& opt) { return opt.has_value(); }\n\n";
  
  out << "template<typename T>\n";
  out << "inline bool isNone(const std::optional<T>& opt) { return !opt.has_value(); }\n\n";
  
  out << "template<typename T>\n";
  out << "inline T unwrap(const std::optional<T>& opt) {\n";
  out << "    if (!opt.has_value()) {\n";
  out << "        throw std::runtime_error(\"Called unwrap on None value\");\n";
  out << "    }\n";
  out << "    return opt.value();\n";
  out << "}\n\n";
  
  out << "template<typename T>\n";
  out << "inline T unwrapOr(const std::optional<T>& opt, const T& defaultValue) {\n";
  out << "    return opt.value_or(defaultValue);\n";
  out << "}\n\n";
  
  out << "#endif // MAGOLOR_STDLIB_HELPERS_H\n\n";
  
  // ============================================================================
  // STEP 3: Generate standard helper wrappers
  // ============================================================================
  out << "// Array helper wrappers\n";
  out << "template<typename T> int length(const ::std::vector<T>& arr) { return "
         "arr.size(); }\n";
  out << "template<typename T> void push(::std::vector<T>& arr, const T& val) { "
         "arr.push_back(val); }\n";
  out << "template<typename T> T pop(::std::vector<T>& arr) { auto v = "
         "arr.back(); arr.pop_back(); return v; }\n";
  out << "\n";
  
  out << "// Map helper wrappers\n";
  out << "namespace Map {\n";
  out << "  template<typename K, typename V> ::std::unordered_map<K,V> create() "
         "{ return {}; }\n";
  out << "  template<typename K, typename V> void "
         "insert(::std::unordered_map<K,V>& m, const K& k, const V& v) { m[k] = "
         "v; }\n";
  out << "  template<typename K, typename V> ::std::optional<V> get(const "
         "::std::unordered_map<K,V>& m, const K& k) {\n";
  out << "    auto it = m.find(k); return it != m.end() ? "
         "::std::optional<V>(it->second) : ::std::nullopt;\n";
  out << "  }\n";
  out << "  template<typename K, typename V> ::std::vector<V> values(const "
         "::std::unordered_map<K,V>& m) {\n";
  out << "    ::std::vector<V> r; for(auto& p : m) r.push_back(p.second); return "
         "r;\n";
  out << "  }\n";
  out << "}\n";
  out << "\n";
  
  out << "// File helper\n";
  out << "namespace File {\n";
  out << "  inline bool exists(const ::std::string& path) {\n";
  out << "    ::std::ifstream f(path); return f.good();\n";
  out << "  }\n";
  out << "}\n";
  out << "\n";
  
  // ============================================================================
  // CRITICAL FIX: Global I/O functions (must be at top level for user code)
  // ============================================================================
  out << "// Global I/O functions\n";
  out << "inline void print(const std::string& s) { std::cout << s; }\n";
  out << "inline void println(const std::string& s) { std::cout << s << std::endl; }\n";
  out << "inline void println() { std::cout << std::endl; }\n";
  out << "inline void eprint(const std::string& s) { std::cerr << s; }\n";
  out << "inline void eprintln(const std::string& s) { std::cerr << s << std::endl; }\n";
  out << "inline std::string readLine() { std::string line; std::getline(std::cin, line); return line; }\n";
  out << "\n";
  out << "// Overloads for common types\n";
  out << "template<typename T>\n";
  out << "inline void print(const T& val) { std::cout << val; }\n";
  out << "template<typename T>\n";
  out << "inline void println(const T& val) { std::cout << val << std::endl; }\n";
  out << "\n";
  
  // ============================================================================
  // STEP 4: Forward declarations for classes
  // ============================================================================
  for (const auto &cls : prog.classes) {
    emitLine("class " + cls.name + ";");
  }
  emitLine("");
  
  // ============================================================================
  // STEP 5: Generate classes
  // ============================================================================
  for (const auto &cls : prog.classes) {
    genClass(cls);
  }
  
  // ============================================================================
  // STEP 6: Forward declarations for functions
  // ============================================================================
  for (const auto &fn : prog.functions) {
    if (fn.name != "main") {
      emit(typeToString(fn.returnType) + " " + fn.name + "(");
      for (size_t i = 0; i < fn.params.size(); i++) {
        if (i > 0)
          emit(", ");
        emit(typeToString(fn.params[i].type) + " " + fn.params[i].name);
      }
      emit(");\n");
    }
  }
  emitLine("");
  
  // ============================================================================
  // STEP 7: Generate function definitions
  // ============================================================================
  for (const auto &fn : prog.functions) {
    genFunction(fn);
    emitLine("");
  }
  
  return out.str();
}




void CodeGen::genClass(const ClassDecl &cls) {
  emitLine("class " + cls.name + " {");

  // Separate public and private
  bool hasPublic = false;
  bool hasPrivate = false;

  // Check what we have
  for (const auto &f : cls.fields) {
    if (f.isPublic)
      hasPublic = true;
    else
      hasPrivate = true;
  }
  for (const auto &m : cls.methods) {
    if (m.isPublic)
      hasPublic = true;
    else
      hasPrivate = true;
  }

  // Generate public section
  if (hasPublic) {
    emitLine("public:");
    indent++;

    // Public static constants
    for (const auto &f : cls.fields) {
      if (f.isPublic && f.isStatic) {
        emitIndent();
        emit("static constexpr " + typeToString(f.type) + " " + f.name + " = ");
        if (f.initValue) {
          genExpr(f.initValue);
        }
        emit(";\n");
      }
    }

    // Public instance fields
    for (const auto &f : cls.fields) {
      if (f.isPublic && !f.isStatic) {
        emitLine(typeToString(f.type) + " " + f.name + ";");
      }
    }

    // Public methods
    for (const auto &m : cls.methods) {
      if (m.isPublic) {
        if (m.isStatic) {
          emit("    static ");
        }
        genFunction(m, cls.name);
      }
    }

    indent--;
  }

  // Generate private section
  if (hasPrivate) {
    emitLine("private:");
    indent++;

    // Private fields
    for (const auto &f : cls.fields) {
      if (!f.isPublic) {
        emitLine(typeToString(f.type) + " " + f.name + ";");
      }
    }

    // Private methods
    for (const auto &m : cls.methods) {
      if (!m.isPublic) {
        genFunction(m, cls.name);
      }
    }

    indent--;
  }

  emitLine("};"); // Close the class definition
  emitLine("");

  // Auto-generate operator<< for printing (OUTSIDE the class)
  // Only print simple types, skip Map/Array/complex types
  emitLine("// Auto-generated print support");
  emitLine("inline std::ostream& operator<<(std::ostream& os, const " +
           cls.name + "& obj) {");
  indent++;
  emitLine("os << \"" + cls.name + " { \";");

  // Print public fields - only simple types
  bool first = true;
  for (const auto &f : cls.fields) {
    if (f.isPublic && !f.isStatic) {
      // Skip complex types that don't have operator<< (Map, Array, other
      // classes)
      bool isSimpleType = false;
      if (f.type) {
        switch (f.type->kind) {
        case Type::INT:
        case Type::FLOAT:
        case Type::STRING:
        case Type::BOOL:
          isSimpleType = true;
          break;
        default:
          isSimpleType = false;
          break;
        }
      }

      if (isSimpleType) {
        if (!first) {
          emitLine("os << \", \";");
        }
        emitLine("os << \"" + f.name + ": \" << obj." + f.name + ";");
        first = false;
      }
    }
  }

  emitLine("os << \" }\";");
  emitLine("return os;");
  indent--;
  emitLine("}");
  emitLine("");
}

void CodeGen::genFunction(const FnDecl &fn, const std::string &className) {
  // Track current class context for proper 'this' handling in return statements
  currentClassName = className;

  std::string retType = typeToString(fn.returnType);
  if (fn.name == "main" && className.empty())
    emitLine("int main() {");
  else if (fn.name == "create" && !className.empty()) {
    // Constructor - special handling
    emitIndent();
    emit("void create(");
    for (size_t i = 0; i < fn.params.size(); i++) {
      if (i > 0)
        emit(", ");
      emit(typeToString(fn.params[i].type) + " " + fn.params[i].name);
    }
    emit(") {\n");
  } else {
    emitIndent();
    emit(retType + " " + fn.name + "(");
    for (size_t i = 0; i < fn.params.size(); i++) {
      if (i > 0)
        emit(", ");
      emit(typeToString(fn.params[i].type) + " " + fn.params[i].name);
    }
    emit(") {\n");
  }
  indent++;
  for (const auto &stmt : fn.body)
    genStmt(stmt);
  if (fn.name == "main" && className.empty())
    emitLine("return 0;");
  indent--;
  emitLine("}");

  // Clear class context after function
  currentClassName.clear();
}

void CodeGen::collectCaptures(const std::vector<StmtPtr> &,
                              const std::vector<Param> &params) {
  capturedVars.clear();
  for (const auto &p : params)
    capturedVars.insert(p.name);
}

void CodeGen::genStmt(const StmtPtr &stmt) {
  std::visit(
      [this](auto &&s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, LetStmt>) {
          emitIndent();
          emit((s.type ? typeToString(s.type) : "auto") + " " + s.name + " = ");
          genExpr(s.init);
          emit(";\n");
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
          emitIndent();
          emit("return");
          if (s.value) {
            emit(" ");
            // FIX: If returning 'this' in a method that returns by value,
            // dereference it
            if (std::holds_alternative<ThisExpr>(s.value->data)) {
              // Check if we're returning from a method (not a free function)
              if (!currentClassName.empty()) {
                // In a method - return *this
                emit("*this");
              } else {
                // In a free function - just emit this (shouldn't happen, but be
                // safe)
                genExpr(s.value);
              }
            } else {
              genExpr(s.value);
            }
          }
          emit(";\n");
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
          emitIndent();
          genExpr(s.expr);
          emit(";\n");
        } else if constexpr (std::is_same_v<T, IfStmt>) {
          emitIndent();
          emit("if (");
          genExpr(s.cond);
          emit(") {\n");
          indent++;
          for (const auto &st : s.thenBody)
            genStmt(st);
          indent--;
          emitLine("}");
          if (!s.elseBody.empty()) {
            emitLine("else {");
            indent++;
            for (const auto &st : s.elseBody)
              genStmt(st);
            indent--;
            emitLine("}");
          }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
          emitIndent();
          emit("while (");
          genExpr(s.cond);
          emit(") {\n");
          indent++;
          for (const auto &st : s.body)
            genStmt(st);
          indent--;
          emitLine("}");
        } else if constexpr (std::is_same_v<T, ForStmt>) {
          emitIndent();
          emit("for (auto& " + s.var + " : ");
          genExpr(s.iterable);
          emit(") {\n");
          indent++;
          for (const auto &st : s.body)
            genStmt(st);
          indent--;
          emitLine("}");
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
          emitIndent();
          emit("{\n");
          indent++;
          emitIndent();
          emit("auto _match_val = ");
          genExpr(s.expr);
          emit(";\n");
          bool first = true;
          for (const auto &arm : s.arms) {
            emitIndent();
            if (!first)
              emit("else ");
            first = false;
            if (arm.pattern == "Some") {
              emit("if (_match_val.has_value()) {\n");
              indent++;
              if (!arm.bindVar.empty())
                emitLine("auto " + arm.bindVar + " = _match_val.value();");
            } else if (arm.pattern == "None") {
              emit("if (!_match_val.has_value()) {\n");
              indent++;
            } else {
              emit("if (_match_val == " + arm.pattern + ") {\n");
              indent++;
            }
            for (const auto &st : arm.body)
              genStmt(st);
            indent--;
            emitLine("}");
          }
          indent--;
          emitLine("}");
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
          emitLine("{");
          indent++;
          for (const auto &st : s.stmts)
            genStmt(st);
          indent--;
          emitLine("}");
        } else if constexpr (std::is_same_v<T, CppStmt>) {
          // FIX 3: Pass through inline C++ blocks WITHOUT transformation
          emitLine("// Inline C++ code:");
          out << s.code;
          if (!s.code.empty() && s.code.back() != '\n') {
            out << "\n";
          }
        }
      },
      stmt->data);
}

void CodeGen::genExpr(const ExprPtr &expr) {
  std::visit(
      [this](auto &&e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, IntLitExpr>)
          emit(std::to_string(e.value));
        else if constexpr (std::is_same_v<T, FloatLitExpr>)
          emit(std::to_string(e.value));
        else if constexpr (std::is_same_v<T, StringLitExpr>) {
          if (e.interpolated) {
            emit("(");
            std::string s = e.value;
            std::string current;
            size_t i = 0;
            bool first = true;
            while (i < s.size()) {
              if (s[i] == '{') {
                if (!current.empty()) {
                  if (!first) {
                    emit(" + ");
                  }
                  first = false;
                  emit("std::string(\"");
                  for (char c : current) {
                    if (c == '\n')
                      emit("\\n");
                    else if (c == '\\')
                      emit("\\\\");
                    else if (c == '"')
                      emit("\\\"");
                    else
                      emit(std::string(1, c));
                  }
                  emit("\")");
                  current.clear();
                }
                i++;
                std::string varName;
                while (i < s.size() && s[i] != '}')
                  varName += s[i++];
                i++;
                if (!first) {
                  emit(" + ");
                }
                first = false;
                emit("mg_to_string(" + varName + ")");
              } else
                current += s[i++];
            }
            if (!current.empty()) {
              if (!first)
                emit(" + ");
              emit("std::string(\"");
              for (char c : current) {
                if (c == '\n')
                  emit("\\n");
                else if (c == '\\')
                  emit("\\\\");
                else if (c == '"')
                  emit("\\\"");
                else
                  emit(std::string(1, c));
              }
              emit("\")");
            }
            emit(")");
          } else {
            emit("std::string(\"");
            for (char c : e.value) {
              if (c == '\n')
                emit("\\n");
              else if (c == '\t')
                emit("\\t");
              else if (c == '\\')
                emit("\\\\");
              else if (c == '"')
                emit("\\\"");
              else
                emit(std::string(1, c));
            }
            emit("\")");
          }
        } else if constexpr (std::is_same_v<T, BoolLitExpr>)
          emit(e.value ? "true" : "false");
        else if constexpr (std::is_same_v<T, IdentExpr>)
          emit(e.name);
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
          emit("(");
          genExpr(e.left);
          emit(" " + e.op + " ");
          genExpr(e.right);
          emit(")");
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
          emit("(" + e.op);
          genExpr(e.operand);
          emit(")");
        } else if constexpr (std::is_same_v<T, CallExpr>) {
          genExpr(e.callee);
          emit("(");
          for (size_t i = 0; i < e.args.size(); i++) {
            if (i > 0)
              emit(", ");
            genExpr(e.args[i]);
          }
          emit(")");
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
          // Check if target is also a MemberExpr (nested like Std.Math.sqrt or Network.HTTP)
          bool isNamespacePath = false;

          // Walk up the chain to see if root is a namespace
          ExprPtr root = e.object;
          while (auto *nested = std::get_if<MemberExpr>(&root->data)) {
            root = nested->object;
          }

          if (auto *rootIdent = std::get_if<IdentExpr>(&root->data)) {
            // FIX 1: Map "Std" to "std" to prevent Std::std double prefix
            if (rootIdent->name == "Std") {
              isNamespacePath = true;
            } else if (importedNamespaces.count(rootIdent->name) > 0) {
              isNamespacePath = true;
            }
          }

          if (isNamespacePath) {
            // For namespace paths, emit the entire chain with ::
            // First, collect the path components
            std::vector<std::string> pathParts;
            ExprPtr current = e.object;
            
            // Walk up collecting member names
            while (auto *member = std::get_if<MemberExpr>(&current->data)) {
              pathParts.push_back(member->member);
              current = member->object;
            }
            
            // Get root identifier
            if (auto *rootIdent = std::get_if<IdentExpr>(&current->data)) {
              if (rootIdent->name == "Std") {
                emit("std");  // FIX: Force lowercase for standard library
              } else {
                emit(rootIdent->name);
              }
            }
            
            // Emit path parts in reverse order (we collected them bottom-up)
            for (auto it = pathParts.rbegin(); it != pathParts.rend(); ++it) {
              emit("::" + *it);
            }
            
            // Finally emit the final member
            emit("::" + e.member);
            return;
          }

          if (auto *ident = std::get_if<IdentExpr>(&e.object->data)) {
            // Check if it's a class name (static access)
            if (isClassName(ident->name)) {
              emit(ident->name + "::" + e.member);
              return;
            }

            // FIX 1: Map Magolor's "Std" to C++'s "std" (lowercase)
            // This prevents Std::std double namespace prefix
            if (ident->name == "Std") {
              emit("std::" + e.member);
              return;
            }
            
            // Check for other namespaces OR any capitalized identifier
            if (ident->name == "std" ||
                importedNamespaces.count(ident->name) > 0 ||
                (ident->name.length() > 0 && std::isupper(ident->name[0]))) {
              // Treat as namespace access - use ::
              emit(ident->name + "::" + e.member);
              return;
            }
          }

          // FIX 2: Handle 'this' pointer access - CRITICAL
          if (std::holds_alternative<ThisExpr>(e.object->data)) {
            emit("this->" + e.member);
            return;  // IMPORTANT: Return early to prevent fall-through
          }

          // Regular member access for value types
          genExpr(e.object);
          emit("." + e.member);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
          genExpr(e.object);
          emit("[");
          genExpr(e.index);
          emit("]");
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
          genExpr(e.target);
          emit(" = ");
          genExpr(e.value);
        } else if constexpr (std::is_same_v<T, LambdaExpr>) {
          emit("[=](");
          for (size_t i = 0; i < e.params.size(); i++) {
            if (i > 0)
              emit(", ");
            emit(paramTypeToString(e.params[i].type) + " " + e.params[i].name);
          }
          emit(")");
          if (e.returnType)
            emit(" -> " + typeToString(e.returnType));
          emit(" {\n");
          indent++;
          for (const auto &st : e.body)
            genStmt(st);
          indent--;
          emitIndent();
          emit("}");
        } else if constexpr (std::is_same_v<T, NewExpr>) {
          emit(e.className + "(");
          for (size_t i = 0; i < e.args.size(); i++) {
            if (i > 0)
              emit(", ");
            genExpr(e.args[i]);
          }
          emit(")");
        } else if constexpr (std::is_same_v<T, SomeExpr>) {
          emit("std::make_optional(");
          genExpr(e.value);
          emit(")");
        } else if constexpr (std::is_same_v<T, NoneExpr>)
          emit("std::nullopt");
        else if constexpr (std::is_same_v<T, ThisExpr>)
          emit("this"); // Just emit pointer - dereferencing happens in ReturnStmt
        else if constexpr (std::is_same_v<T, ArrayExpr>) {
          // Determine element type
          std::string elemType = "int";
          if (!e.elements.empty() && e.elements[0]->type) {
            elemType = typeToString(e.elements[0]->type);
          }

          // Check if this looks like an enum type
          bool isEnum = false;
          if (!e.elements.empty()) {
            // Check if first element is a member access with ::
            if (auto *member = std::get_if<MemberExpr>(&e.elements[0]->data)) {
              // This is likely an enum value
              isEnum = true;
            }
          }

          emit("std::vector<" + elemType + ">{");
          for (size_t i = 0; i < e.elements.size(); i++) {
            if (i > 0)
              emit(", ");
            genExpr(e.elements[i]);
          }
          emit("}");
        }
      },
      expr->data);
}

std::string CodeGen::paramTypeToString(const TypePtr &type) {
  // For untyped lambda parameters (type is VOID from parser), use "auto"
  if (!type || type->kind == Type::VOID) {
    return "auto";
  }
  // For explicitly typed parameters, use normal type conversion
  return typeToString(type);
}

void CodeGen::enterScope() {
  // Placeholder for future scope management
}

void CodeGen::exitScope() {
  // Placeholder for future scope management
}

void CodeGen::registerVar(const std::string &name, const std::string &type,
                          bool isMut) {
  scopeVars[name] = {type, isMut};
}
