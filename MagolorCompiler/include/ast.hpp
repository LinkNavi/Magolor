#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>

struct Type;
struct Expr;
struct Stmt;

using TypePtr = std::shared_ptr<Type>;
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct Type {
    enum Kind { INT, FLOAT, STRING, BOOL, VOID, FUNCTION, CLASS, OPTION, ARRAY };
    Kind kind;
    std::string className;
    TypePtr returnType;
    std::vector<TypePtr> paramTypes;
    TypePtr innerType; // for Option<T>, Array<T>
};

struct Param {
    std::string name;
    TypePtr type;
};

// Expression nodes
struct IntLitExpr { int value; };
struct FloatLitExpr { double value; };
struct StringLitExpr { std::string value; bool interpolated; };
struct BoolLitExpr { bool value; };
struct IdentExpr { std::string name; };
struct BinaryExpr { std::string op; ExprPtr left, right; };
struct UnaryExpr { std::string op; ExprPtr operand; };
struct CallExpr { ExprPtr callee; std::vector<ExprPtr> args; };
struct MemberExpr { ExprPtr object; std::string member; };
struct IndexExpr { ExprPtr object; ExprPtr index; };
struct AssignExpr { ExprPtr target; ExprPtr value; };
struct LambdaExpr { std::vector<Param> params; TypePtr returnType; std::vector<StmtPtr> body; };
struct NewExpr { std::string className; std::vector<ExprPtr> args; };
struct SomeExpr { ExprPtr value; };
struct NoneExpr {};
struct ThisExpr {};
struct ArrayExpr { std::vector<ExprPtr> elements; };

using ExprVariant = std::variant<
    IntLitExpr, FloatLitExpr, StringLitExpr, BoolLitExpr, IdentExpr,
    BinaryExpr, UnaryExpr, CallExpr, MemberExpr, IndexExpr, AssignExpr,
    LambdaExpr, NewExpr, SomeExpr, NoneExpr, ThisExpr, ArrayExpr
>;

struct Expr {
    ExprVariant data;
    TypePtr type; // filled in by type checker
};

// Statement nodes
struct LetStmt { std::string name; TypePtr type; ExprPtr init; bool isMut; };
struct ReturnStmt { ExprPtr value; };
struct ExprStmt { ExprPtr expr; };
struct IfStmt { ExprPtr cond; std::vector<StmtPtr> thenBody; std::vector<StmtPtr> elseBody; };
struct WhileStmt { ExprPtr cond; std::vector<StmtPtr> body; };
struct ForStmt { std::string var; ExprPtr iterable; std::vector<StmtPtr> body; };
struct MatchArm { std::string pattern; std::string bindVar; std::vector<StmtPtr> body; };
struct MatchStmt { ExprPtr expr; std::vector<MatchArm> arms; };
struct BlockStmt { std::vector<StmtPtr> stmts; };
struct CppStmt { std::string code; };  // Inline C++ code block

using StmtVariant = std::variant<
    LetStmt, ReturnStmt, ExprStmt, IfStmt, WhileStmt, ForStmt, MatchStmt, BlockStmt, CppStmt
>;

struct Stmt {
    StmtVariant data;
};

// Top-level declarations
struct FnDecl {
    std::string name;
    std::vector<Param> params;
    TypePtr returnType;
    std::vector<StmtPtr> body;
    bool isPublic;  // true if marked with 'pub'
    bool isStatic;  // true if marked with 'static'
};

struct Field {
    std::string name;
    TypePtr type;
    bool isPublic;  // true if marked with 'pub'
    bool isStatic;  // true if marked with 'static'
    ExprPtr initValue;  // for static const initialization
};

struct ClassDecl {
    std::string name;
    std::vector<Field> fields;
    std::vector<FnDecl> methods;
    std::string parent;
    bool isPublic;  // classes can be pub (for exporting from modules)
};

struct UsingDecl {
    std::vector<std::string> path; // e.g., ["Std", "IO"] or ["Math"]
    bool isWildcard;  // for future use (using Math.*)
};

// C/C++ import declaration
struct CImportDecl {
    std::string header;          // e.g., "stdio.h", "vector"
    bool isSystemHeader;         // <header> vs "header"
    std::string asNamespace;     // optional namespace alias
    std::vector<std::string> symbols; // specific symbols to import (empty = all)
};

// Module declaration (represents a .mg file)
struct ModuleDecl {
    std::string name;  // Module name (derived from filename)
    std::vector<ClassDecl> exports;  // Public classes that can be imported
};

struct Program {
    std::vector<UsingDecl> usings;
    std::vector<CImportDecl> cimports;  // C/C++ imports
    std::vector<ClassDecl> classes;
    std::vector<FnDecl> functions;
    std::string moduleName;  // Name of this module (from filename)
};
