#pragma once

#include "common/helpers.h"

namespace tiny {

    inline constexpr char const EmptyPrefix[] = "";

    /** Simple base for objects that have human readable-ish names. 
     */
    template<char const * const PREFIX = EmptyPrefix>
    class NamedEntity {
    public:

        NamedEntity():
            name_{STR(PREFIX << (id_++))} {
        }

        NamedEntity(std::string const & name):
            name_{name} {
        }

        std::string const & name() const { return name_; }

        void setName(std::string const & name) { name_ = name; }

    private:
        static size_t id_;

        std::string name_;
    };


    template<char const * const PREFIX>
    size_t NamedEntity<PREFIX>::id_ = 0;
}