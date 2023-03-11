#pragma once
#include <string>
#include <vector>
#include <variant>

/// Represents 
struct TypeInfo {
    std::string name;
    int size;
};

class PrimitiveType;
class StructuredType;

using Type = std::variant<PrimitiveType, StructuredType>;

class PrimitiveType: public TypeInfo {
};

class StructuredType: public TypeInfo {
    /// Offset from base of structured type - The type at that offset
    std::vector<std::pair<int64_t, Type>> members;
};
