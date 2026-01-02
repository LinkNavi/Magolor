#include "codegen.hpp"
#include "stdlib.hpp"
#include <unordered_set>
#include <variant>

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

  // Collect all class names first
  for (const auto &cls : prog.classes) {
    knownClassNames.insert(cls.name);
  }

  // Generate C/C++ imports first
  genCImports(prog.cimports);

  // Generate standard library
  genStdLib();

  // Forward declarations for classes
  for (const auto &cls : prog.classes) {
    emitLine("class " + cls.name + ";");
  }
  emitLine("");

  // Generate classes
  for (const auto &cls : prog.classes) {
    genClass(cls);
  }

  // Forward declarations for functions
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

  // Generate function definitions
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

  emitLine("};");
  emitLine("");
}

void CodeGen::genFunction(const FnDecl &fn, const std::string &className) {
  std::string retType = typeToString(fn.returnType);
  if (fn.name == "main" && className.empty())
    emitLine("int main() {");
  else {
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
            genExpr(s.value);
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
          // Check if target is also a MemberExpr (nested like Std.Math.sqrt or
          // Network.HTTP)
          bool isNamespacePath = false;

          // Walk up the chain to see if root is a namespace
          ExprPtr root = e.object;
          while (auto *nested = std::get_if<MemberExpr>(&root->data)) {
            root = nested->object;
          }

          if (auto *rootIdent = std::get_if<IdentExpr>(&root->data)) {
            if (rootIdent->name == "Std" ||
                importedNamespaces.count(rootIdent->name) > 0) {
              isNamespacePath = true;
            }
          }

          if (isNamespacePath) {
            // Emit entire path with ::
            genExpr(e.object);
            emit("::" + e.member);
            return;
          }

          if (auto *ident = std::get_if<IdentExpr>(&e.object->data)) {
            // Check if it's a class name (static access)
            if (isClassName(ident->name)) {
              emit(ident->name + "::" + e.member);
              return;
            }

            // Check for built-in namespaces OR any capitalized identifier
            // followed by a member This allows Network.HTTP, Network.Security,
            // etc. to work
            if (ident->name == "Std" || ident->name == "std" ||
                importedNamespaces.count(ident->name) > 0 ||
                (ident->name.length() > 0 && std::isupper(ident->name[0]))) {
              // Treat as namespace access - use ::
              emit(ident->name + "::" + e.member);
              return;
            }
          }

          // Regular member access - use -> for pointers
          genExpr(e.object);

          // Check if object is 'this' - use -> instead of .
          if (std::holds_alternative<ThisExpr>(e.object->data)) {
            emit("->" + e.member);
          } else {
            emit("." + e.member);
          }
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
          emit("this"); // Changed from (*this)
        else if constexpr (std::is_same_v<T, ArrayExpr>) {
          // Determine element type
          std::string elemType = "int";
          if (!e.elements.empty() && e.elements[0]->type) {
            elemType = typeToString(e.elements[0]->type);
          }

          // NEW: Check if this looks like an enum type
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
