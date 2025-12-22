#pragma once
#include "position.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class SymbolKind {
  Function = 12,
  Variable = 13,
  Class = 5,
  Field = 8,
  Method = 6,
  Parameter = 25,
  Module = 2
};

struct Symbol {
  std::string name;
  SymbolKind kind;
  std::string type;
  Location definition;
  std::vector<Location> references;
  std::string documentation;
  std::string detail;
  bool isPublic = true;
  bool isStatic = false;
  std::string containerName;
};

using SymbolPtr = std::shared_ptr<Symbol>;

class SymbolTable {
public:
  void addSymbol(SymbolPtr sym) {
    symbols[sym->name] = sym;
    byLocation[locationKey(sym->definition)] = sym;
  }

  SymbolPtr lookup(const std::string &name) {
    auto it = symbols.find(name);
    return it != symbols.end() ? it->second : nullptr;
  }

  // In symbol_table.cpp
  std::vector<std::shared_ptr<Symbol>> getAllSymbols() const {
    std::vector<std::shared_ptr<Symbol>> result;
    for (const auto &pair : fileSymbols) {
      result.insert(result.end(), pair.second.begin(), pair.second.end());
    }
    return result;
  }
  SymbolPtr findAtPosition(const std::string &uri, const Position &pos) {
    for (auto &[_, sym] : symbols) {
      if (sym->definition.uri == uri && sym->definition.range.contains(pos)) {
        return sym;
      }
      for (auto &ref : sym->references) {
        if (ref.uri == uri && ref.range.contains(pos)) {
          return sym;
        }
      }
    }
    return nullptr;
  }

  void addReference(const std::string &name, const Location &loc) {
    auto sym = lookup(name);
    if (sym)
      sym->references.push_back(loc);
  }

  std::vector<SymbolPtr> getSymbolsInFile(const std::string &uri) {
    std::vector<SymbolPtr> result;
    for (auto &[_, sym] : symbols) {
      if (sym->definition.uri == uri) {
        result.push_back(sym);
      }
    }
    return result;
  }

  std::vector<SymbolPtr> findByPrefix(const std::string &prefix) {
    std::vector<SymbolPtr> result;
    for (auto &[name, sym] : symbols) {
      if (name.find(prefix) == 0) {
        result.push_back(sym);
      }
    }
    return result;
  }

  void clear() {
    symbols.clear();
    byLocation.clear();
  }

  const std::unordered_map<std::string, SymbolPtr> &all() const {
    return symbols;
  }

private:
  std::unordered_map<std::string, SymbolPtr> symbols;
  std::unordered_map<std::string, SymbolPtr> byLocation;

  std::unordered_map<std::string, std::vector<std::shared_ptr<Symbol>>>
      fileSymbols;
  std::string locationKey(const Location &loc) {
    return loc.uri + ":" + std::to_string(loc.range.start.line) + ":" +
           std::to_string(loc.range.start.character);
  }
};
