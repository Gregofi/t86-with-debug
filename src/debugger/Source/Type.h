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
struct ArrayType;

using Type = std::variant<PrimitiveType, StructuredType, PointerType, ArrayType>;

struct PrimitiveType {
    enum class Type {
        FLOAT,
        SIGNED,
        UNSIGNED,
        CHAR,
    };
    Type type;
    uint64_t size;
};

inline std::optional<PrimitiveType::Type> ToPrimitiveType(std::string_view type) {
    if (type == "float") {
        return PrimitiveType::Type::FLOAT; 
    } else if (type == "int") {
        return PrimitiveType::Type::SIGNED;
    } else if (type == "unsigned") {
        return PrimitiveType::Type::UNSIGNED;
    } else if (type == "char") {
        return PrimitiveType::Type::CHAR;
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
    case PrimitiveType::Type::CHAR:
        return "char";
    }
    UNREACHABLE;
}

struct PointerType {
    // Ptr can be recursive with structs, so we just
    // store the id of the type  it points to.
    size_t type_id;
    uint64_t size;
};

struct ArrayType {
    size_t type_id;
    uint64_t cnt;
};

struct StructuredMember;

struct StructuredType {
    std::string name;
    uint64_t size;
    std::vector<StructuredMember> members;
};

struct StructuredMember {
    std::string name;
    size_t type_id;
    int64_t offset;
};
