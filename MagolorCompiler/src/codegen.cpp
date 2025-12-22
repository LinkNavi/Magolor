#include "codegen.hpp"
#include <variant>

void CodeGen::emit(const std::string& s) { out << s; }
void CodeGen::emitLine(const std::string& s) { emitIndent(); out << s << "\n"; }
void CodeGen::emitIndent() { for (int i = 0; i < indent; i++) out << "    "; }

std::string CodeGen::typeToString(const TypePtr& type) {
    if (!type) return "auto";
    switch (type->kind) {
        case Type::INT: return "int";
        case Type::FLOAT: return "double";
        case Type::STRING: return "std::string";
        case Type::BOOL: return "bool";
        case Type::VOID: return "void";
        case Type::CLASS: return type->className;
        case Type::OPTION: return "std::optional<" + typeToString(type->innerType) + ">";
        case Type::ARRAY: return "std::vector<" + typeToString(type->innerType) + ">";
        case Type::FUNCTION: {
            std::string s = "std::function<" + typeToString(type->returnType) + "(";
            for (size_t i = 0; i < type->paramTypes.size(); i++) {
                if (i > 0) s += ", ";
                s += typeToString(type->paramTypes[i]);
            }
            return s + ")>";
        }
    }
    return "auto";
}

void CodeGen::genStdLib() {
    emitLine("#include <iostream>");
    emitLine("#include <string>");
    emitLine("#include <functional>");
    emitLine("#include <optional>");
    emitLine("#include <vector>");
    emitLine("#include <sstream>");
    emitLine("#include <stdexcept>");
    emitLine("");
    emitLine("namespace Std {");
    indent++;
    emitLine("namespace IO {");
    indent++;
    emitLine("inline void print(const std::string& s) { std::cout << s; }");
    emitLine("inline void println(const std::string& s) { std::cout << s << std::endl; }");
    emitLine("inline std::string readLine() { std::string line; std::getline(std::cin, line); return line; }");
    indent--;
    emitLine("}");
    emitLine("");
    emitLine("namespace Parse {");
    indent++;
    emitLine("inline std::optional<int> parseInt(const std::string& s) {");
    indent++;
    emitLine("try { size_t pos; int val = std::stoi(s, &pos); if (pos == s.length()) return val; return std::nullopt; }");
    emitLine("catch (...) { return std::nullopt; }");
    indent--;
    emitLine("}");
    emitLine("inline std::optional<double> parseFloat(const std::string& s) {");
    indent++;
    emitLine("try { size_t pos; double val = std::stod(s, &pos); if (pos == s.length()) return val; return std::nullopt; }");
    emitLine("catch (...) { return std::nullopt; }");
    indent--;
    emitLine("}");
    indent--;
    emitLine("}");
    emitLine("");
    emitLine("namespace Option {");
    indent++;
    emitLine("template<typename T> bool isSome(const std::optional<T>& opt) { return opt.has_value(); }");
    emitLine("template<typename T> bool isNone(const std::optional<T>& opt) { return !opt.has_value(); }");
    emitLine("template<typename T> T unwrap(const std::optional<T>& opt) { return opt.value(); }");
    indent--;
    emitLine("}");
    emitLine("");
    emitLine("inline void print(const std::string& s) { IO::print(s); }");
    emitLine("inline void println(const std::string& s) { IO::println(s); }");
    emitLine("inline std::string readLine() { return IO::readLine(); }");
    emitLine("inline std::optional<int> parseInt(const std::string& s) { return Parse::parseInt(s); }");
    emitLine("inline std::optional<double> parseFloat(const std::string& s) { return Parse::parseFloat(s); }");
    indent--;
    emitLine("}");
    emitLine("");
    emitLine("template<typename T> std::string mg_to_string(const T& val) { std::ostringstream oss; oss << val; return oss.str(); }");
    emitLine("");
}

std::string CodeGen::generate(const Program& prog) {
    out.str(""); out.clear();
    for (const auto& u : prog.usings) if (!u.path.empty()) usedModules.insert(u.path[0]);
    genStdLib();
    for (const auto& cls : prog.classes) genClass(cls);
    for (const auto& fn : prog.functions) {
        if (fn.name != "main") {
            emit(typeToString(fn.returnType) + " " + fn.name + "(");
            for (size_t i = 0; i < fn.params.size(); i++) {
                if (i > 0) emit(", ");
                emit(typeToString(fn.params[i].type) + " " + fn.params[i].name);
            }
            emit(");\n");
        }
    }
    emitLine("");
    for (const auto& fn : prog.functions) { genFunction(fn); emitLine(""); }
    return out.str();
}

void CodeGen::genClass(const ClassDecl& cls) {
    emitLine("class " + cls.name + " {");
    emitLine("public:");
    indent++;
    for (const auto& f : cls.fields) emitLine(typeToString(f.type) + " " + f.name + ";");
    emit("    " + cls.name + "(");
    for (size_t i = 0; i < cls.fields.size(); i++) {
        if (i > 0) emit(", ");
        emit(typeToString(cls.fields[i].type) + " _" + cls.fields[i].name);
    }
    emit(") : ");
    for (size_t i = 0; i < cls.fields.size(); i++) {
        if (i > 0) emit(", ");
        emit(cls.fields[i].name + "(_" + cls.fields[i].name + ")");
    }
    emit(" {}\n");
    if (cls.fields.empty()) emitLine(cls.name + "() {}");
    for (const auto& m : cls.methods) genFunction(m, cls.name);
    indent--;
    emitLine("};");
    emitLine("");
}

void CodeGen::genFunction(const FnDecl& fn, const std::string& className) {
    std::string retType = typeToString(fn.returnType);
    if (fn.name == "main" && className.empty()) emitLine("int main() {");
    else {
        emitIndent();
        emit(retType + " " + fn.name + "(");
        for (size_t i = 0; i < fn.params.size(); i++) {
            if (i > 0) emit(", ");
            emit(typeToString(fn.params[i].type) + " " + fn.params[i].name);
        }
        emit(") {\n");
    }
    indent++;
    for (const auto& stmt : fn.body) genStmt(stmt);
    if (fn.name == "main" && className.empty()) emitLine("return 0;");
    indent--;
    emitLine("}");
}

void CodeGen::collectCaptures(const std::vector<StmtPtr>&, const std::vector<Param>& params) {
    capturedVars.clear();
    for (const auto& p : params) capturedVars.insert(p.name);
}

void CodeGen::genStmt(const StmtPtr& stmt) {
    std::visit([this](auto&& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, LetStmt>) {
            emitIndent();
            emit((s.type ? typeToString(s.type) : "auto") + " " + s.name + " = ");
            genExpr(s.init);
            emit(";\n");
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            emitIndent(); emit("return");
            if (s.value) { emit(" "); genExpr(s.value); }
            emit(";\n");
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            emitIndent(); genExpr(s.expr); emit(";\n");
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            emitIndent(); emit("if ("); genExpr(s.cond); emit(") {\n");
            indent++;
            for (const auto& st : s.thenBody) genStmt(st);
            indent--;
            emitLine("}");
            if (!s.elseBody.empty()) {
                emitLine("else {");
                indent++;
                for (const auto& st : s.elseBody) genStmt(st);
                indent--;
                emitLine("}");
            }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            emitIndent(); emit("while ("); genExpr(s.cond); emit(") {\n");
            indent++;
            for (const auto& st : s.body) genStmt(st);
            indent--;
            emitLine("}");
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            emitIndent(); emit("for (auto& " + s.var + " : "); genExpr(s.iterable); emit(") {\n");
            indent++;
            for (const auto& st : s.body) genStmt(st);
            indent--;
            emitLine("}");
        } else if constexpr (std::is_same_v<T, MatchStmt>) {
            emitIndent(); emit("{\n"); indent++;
            emitIndent(); emit("auto _match_val = "); genExpr(s.expr); emit(";\n");
            bool first = true;
            for (const auto& arm : s.arms) {
                emitIndent();
                if (!first) emit("else ");
                first = false;
                if (arm.pattern == "Some") {
                    emit("if (_match_val.has_value()) {\n"); indent++;
                    if (!arm.bindVar.empty()) emitLine("auto " + arm.bindVar + " = _match_val.value();");
                } else if (arm.pattern == "None") {
                    emit("if (!_match_val.has_value()) {\n"); indent++;
                } else {
                    emit("if (_match_val == " + arm.pattern + ") {\n"); indent++;
                }
                for (const auto& st : arm.body) genStmt(st);
                indent--; emitLine("}");
            }
            indent--; emitLine("}");
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
            emitLine("{"); indent++;
            for (const auto& st : s.stmts) genStmt(st);
            indent--; emitLine("}");
        }
    }, stmt->data);
}

void CodeGen::genExpr(const ExprPtr& expr) {
    std::visit([this](auto&& e) {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, IntLitExpr>) emit(std::to_string(e.value));
        else if constexpr (std::is_same_v<T, FloatLitExpr>) emit(std::to_string(e.value));
        else if constexpr (std::is_same_v<T, StringLitExpr>) {
            if (e.interpolated) {
                emit("(");
                std::string s = e.value; std::string current; size_t i = 0; bool first = true;
                while (i < s.size()) {
                    if (s[i] == '{') {
                        if (!current.empty()) {
                            if (!first) emit(" + "); first = false;
                            emit("std::string(\"");
                            for (char c : current) {
                                if (c == '\n') emit("\\n"); else if (c == '\\') emit("\\\\");
                                else if (c == '"') emit("\\\""); else emit(std::string(1, c));
                            }
                            emit("\")"); current.clear();
                        }
                        i++; std::string varName;
                        while (i < s.size() && s[i] != '}') varName += s[i++];
                        i++;
                        if (!first) emit(" + "); first = false;
                        emit("mg_to_string(" + varName + ")");
                    } else current += s[i++];
                }
                if (!current.empty()) {
                    if (!first) emit(" + ");
                    emit("std::string(\"");
                    for (char c : current) {
                        if (c == '\n') emit("\\n"); else if (c == '\\') emit("\\\\");
                        else if (c == '"') emit("\\\""); else emit(std::string(1, c));
                    }
                    emit("\")");
                }
                emit(")");
            } else {
                emit("std::string(\"");
                for (char c : e.value) {
                    if (c == '\n') emit("\\n"); else if (c == '\t') emit("\\t");
                    else if (c == '\\') emit("\\\\"); else if (c == '"') emit("\\\"");
                    else emit(std::string(1, c));
                }
                emit("\")");
            }
        }
        else if constexpr (std::is_same_v<T, BoolLitExpr>) emit(e.value ? "true" : "false");
        else if constexpr (std::is_same_v<T, IdentExpr>) emit(e.name);
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            emit("("); genExpr(e.left); emit(" " + e.op + " "); genExpr(e.right); emit(")");
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) { emit("(" + e.op); genExpr(e.operand); emit(")"); }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            genExpr(e.callee); emit("(");
            for (size_t i = 0; i < e.args.size(); i++) { if (i > 0) emit(", "); genExpr(e.args[i]); }
            emit(")");
        }
        else if constexpr (std::is_same_v<T, MemberExpr>) {
            // Check if it's a namespace access (like Std.print)
            if (auto* ident = std::get_if<IdentExpr>(&e.object->data)) {
                if (ident->name == "Std") {
                    emit("Std::" + e.member);
                    return;
                }
            }
            genExpr(e.object); emit("." + e.member);
        }
        else if constexpr (std::is_same_v<T, IndexExpr>) { genExpr(e.object); emit("["); genExpr(e.index); emit("]"); }
        else if constexpr (std::is_same_v<T, LambdaExpr>) {
            emit("[=](");
            for (size_t i = 0; i < e.params.size(); i++) {
                if (i > 0) emit(", ");
                emit(typeToString(e.params[i].type) + " " + e.params[i].name);
            }
            emit(")");
            if (e.returnType) emit(" -> " + typeToString(e.returnType));
            emit(" {\n"); indent++;
            for (const auto& st : e.body) genStmt(st);
            indent--; emitIndent(); emit("}");
        }
        else if constexpr (std::is_same_v<T, NewExpr>) {
            emit(e.className + "(");
            for (size_t i = 0; i < e.args.size(); i++) { if (i > 0) emit(", "); genExpr(e.args[i]); }
            emit(")");
        }
        else if constexpr (std::is_same_v<T, SomeExpr>) { emit("std::make_optional("); genExpr(e.value); emit(")"); }
        else if constexpr (std::is_same_v<T, NoneExpr>) emit("std::nullopt");
        else if constexpr (std::is_same_v<T, ThisExpr>) emit("(*this)");
        else if constexpr (std::is_same_v<T, ArrayExpr>) {
            emit("{");
            for (size_t i = 0; i < e.elements.size(); i++) { if (i > 0) emit(", "); genExpr(e.elements[i]); }
            emit("}");
        }
    }, expr->data);
}
