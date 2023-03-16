#pragma once
#include <map>
#include <string>
#include <vector>
#include "LocExpr.h"

struct ATTR_name {
    std::string n;
};

struct ATTR_begin_addr {
    uint64_t addr;
};

struct ATTR_end_addr {
    uint64_t addr;
};

struct ATTR_type {
    size_t type_id;
};

struct ATTR_location_expr {
    std::vector<expr::LocExpr> locs;
};

struct ATTR_size {
    uint64_t size;
};

struct ATTR_id {
    size_t id;
};

struct ATTR_member {
    std::string name;
    size_t type_id;
    int64_t offset;
};

struct ATTR_members {
    /// address offset from struct beginning -> name of type
    std::vector<ATTR_member> m;
};

using DIE_ATTR = std::variant<ATTR_name,
                              ATTR_begin_addr,
                              ATTR_end_addr,
                              ATTR_type,
                              ATTR_location_expr,
                              ATTR_size,
                              ATTR_members,
                              ATTR_id>;

/// Represents one DIE_TAG value. That value contains attributes
/// and possibly other values.
class DIE {
public:
    enum class TAG {
        function,
        scope,
        variable,
        primitive_type,
        structured_type,
        pointer_type,
        name,
        invalid,
        compilation_unit,
    };

    DIE(TAG tag,
            std::vector<DIE_ATTR> attributes,
            std::vector<DIE> children)
        : tag(tag),
          attributes(std::move(attributes)),
          children(std::move(children)) {}

    /// Begin iterator to the children DIEs.
    auto begin() const {
        return children.cbegin();
    }

    /// Past the end iterator to child DIEs.
    auto end() const {
        return children.cend();
    }

    auto begin_attr() const {
        return attributes.begin();
    }

    auto end_attr() const {
        return attributes.end();
    }

    const DIE& at(size_t i) {
        return children.at(i);
    }
    
    const DIE_ATTR& attr_at(size_t i) {
        return attributes.at(i);
    }

    TAG get_tag() const { return tag; }
protected:
    TAG tag;
    std::vector<DIE_ATTR> attributes;
    std::vector<DIE> children;
};

