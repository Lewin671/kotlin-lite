#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

namespace kotlin_lite {

enum class SymbolType {
    INT,
    BOOLEAN,
    UNIT,
    FLOAT,  // Grammar allows it, but backend might not
    STRING, // Grammar allows it, but backend might not
    UNKNOWN
};

inline std::string to_string(SymbolType type) {
    switch (type) {
        case SymbolType::INT: return "Int";
        case SymbolType::BOOLEAN: return "Boolean";
        case SymbolType::UNIT: return "Unit";
        case SymbolType::FLOAT: return "Float";
        case SymbolType::STRING: return "String";
        default: return "Unknown";
    }
}

inline SymbolType string_to_type(const std::string& name) {
    if (name == "Int") return SymbolType::INT;
    if (name == "Boolean") return SymbolType::BOOLEAN;
    if (name == "Unit") return SymbolType::UNIT;
    if (name == "Float") return SymbolType::FLOAT;
    if (name == "String") return SymbolType::STRING;
    return SymbolType::UNKNOWN;
}

struct VariableSymbol {
    std::string name;
    SymbolType type;
    bool is_val;
    int line;
    int column;
};

struct FunctionSymbol {
    std::string name;
    std::vector<SymbolType> parameter_types;
    SymbolType return_type;
    int line;
    int column;
};

class SymbolTable {
public:
    SymbolTable() {
        // Global scope
        scopes_.emplace_back();
    }

    void enterScope() {
        scopes_.emplace_back();
    }

    void exitScope() {
        if (scopes_.size() > 1) {
            scopes_.pop_back();
        }
    }

    bool declareVariable(const std::string& name, SymbolType type, bool is_val, int line, int column) {
        if (scopes_.back().variables.count(name)) return false;
        scopes_.back().variables[name] = {name, type, is_val, line, column};
        return true;
    }

    bool declareFunction(const std::string& name, std::vector<SymbolType> params, SymbolType ret, int line, int column) {
        // Functions are always global in our subset
        if (scopes_.front().functions.count(name)) return false;
        scopes_.front().functions[name] = {name, std::move(params), ret, line, column};
        return true;
    }

    std::optional<VariableSymbol> lookupVariable(const std::string& name) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->variables.find(name);
            if (found != it->variables.end()) return found->second;
        }
        return std::nullopt;
    }

    std::optional<FunctionSymbol> lookupFunction(const std::string& name) {
        auto found = scopes_.front().functions.find(name);
        if (found != scopes_.front().functions.end()) return found->second;
        return std::nullopt;
    }

private:
    struct Scope {
        std::map<std::string, VariableSymbol> variables;
        std::map<std::string, FunctionSymbol> functions;
    };
    std::vector<Scope> scopes_;
};

} // namespace kotlin_lite
