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
    // Ptr can be recursive with structs, so we just
    // store the id of the type  it points to.
    size_t type_idx;
    // Keep the name of the pointed type for convenience.
    std::string name;
    uint64_t size;
};

struct StructuredMember;

struct StructuredType {
    std::string name;
    // If the size is zero then this is considered forward declaration
    // TODO: Maybe add separate variable, size zero is forbidden in C
    // but can be in other languages.
    uint64_t size;
    std::vector<StructuredMember> members;
};

struct StructuredMember {
    std::string name;
    std::optional<Type> type;
    int64_t offset;
};

inline std::string TypeToString(const Type& type) {
    return std::visit(utils::overloaded {
        [](const PrimitiveType& t) { return FromPrimitiveType(t.type); },
        [](const PointerType& t) {
            return fmt::format("{}*", t.name);
        },
        [](const StructuredType& t) {
            return t.name;
        }
    }, type);
}
