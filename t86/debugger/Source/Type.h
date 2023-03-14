#pragma once
#include "fmt/core.h"
#include "helpers.h"
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <optional>

struct PrimitiveType;
struct StructuredType;
struct PointerType;

using Type = std::variant<PrimitiveType, StructuredType, PointerType>;

struct PrimitiveType {
    enum class Type {
        FLOAT,
        SIGNED,
        UNSIGNED,
        BOOL,
    };
    Type type;
    uint64_t size;
};

inline std::optional<PrimitiveType::Type> ToPrimitiveType(std::string_view type) {
    if (type == "float") {
        return PrimitiveType::Type::FLOAT; 
    } else if (type == "signed_int") {
        return PrimitiveType::Type::SIGNED;
    } else if (type == "unsigned_int") {
        return PrimitiveType::Type::UNSIGNED;
    } else if (type == "bool") {
        return PrimitiveType::Type::BOOL;
    }
    return {};
}

inline std::string FromPrimitiveType(PrimitiveType::Type type) {
    switch (type) {
    case PrimitiveType::Type::FLOAT:
        return "float";
    case PrimitiveType::Type::SIGNED:
        return "int";
    case PrimitiveType::Type::UNSIGNED:
        return "unsigned";
    case PrimitiveType::Type::BOOL:
        return "bool";
    }
    UNREACHABLE;
}


struct PointerType {
    // ptr is needed here since the types are recursive.
    // Can be null if there is no info about the pointed type.
    std::shared_ptr<Type> to;
    uint64_t size;
};

struct StructuredType {
    std::string name;
    uint64_t size;
    /// Offset from base of structured type - The type at that offset
    std::vector<std::pair<int64_t, std::optional<Type>>> members;
};

inline std::string TypeToString(const Type& type) {
    return std::visit(utils::overloaded {
        [](const PrimitiveType& t) { return FromPrimitiveType(t.type); },
        [](const PointerType& t) {
            auto pointed = TypeToString(*t.to);
            return fmt::format("{}*", pointed);
        },
        [](const StructuredType& t) {
            return t.name;
        }
    }, type);
}
