#pragma once

#include <ostream>
#include <vector>
#include <unordered_map>


namespace tiny {

    class Symbol {
    public:

        static Symbol const Inc;
        static Symbol const Dec;
        static Symbol const Add;
        static Symbol const Sub;
        static Symbol const Mul;
        static Symbol const Div;
        static Symbol const Mod;
        static Symbol const ShiftLeft;
        static Symbol const ShiftRight;
        static Symbol const Eq;
        static Symbol const NEq;
        static Symbol const Lt;
        static Symbol const Gt;
        static Symbol const Lte;
        static Symbol const Gte;
        static Symbol const BitAnd;
        static Symbol const BitOr;
        static Symbol const And;
        static Symbol const Or;
        static Symbol const Not;
        static Symbol const Neg;
        static Symbol const Xor;
        static Symbol const Dot;
        static Symbol const Semicolon;
        static Symbol const Colon;
        static Symbol const ArrowL;
        static Symbol const ArrowR;
        static Symbol const Comma;
        static Symbol const ParOpen;
        static Symbol const ParClose;
        static Symbol const SquareOpen;
        static Symbol const SquareClose;
        static Symbol const CurlyOpen;
        static Symbol const CurlyClose;
        static Symbol const Assign;
        static Symbol const Backtick;

        static Symbol const KwBreak;
        static Symbol const KwCase;
        static Symbol const KwCast;
        static Symbol const KwChar;
        static Symbol const KwContinue;
        static Symbol const KwDefault;
        static Symbol const KwDefine;
        static Symbol const KwDefmacro;
        static Symbol const KwDo;
        static Symbol const KwDouble;
        static Symbol const KwElse;
        static Symbol const KwFor;
        static Symbol const KwIf;
        static Symbol const KwInt;
        static Symbol const KwReturn;
        static Symbol const KwStruct;
        static Symbol const KwSwitch;
        static Symbol const KwTypedef;
        static Symbol const KwVoid;
        static Symbol const KwWhile;

        explicit Symbol(std::string const & name) {
            Symbols & s{Symbols_()};
            auto i = s.lookup.find(name);
            if (i == s.lookup.end()) {
                i = s.lookup.insert(std::make_pair(name, s.names.size())).first;
                s.names.push_back(name);
            }
            id_ = i->second;
        }

        Symbol(Symbol const & other) = default;

        std::string const & name() const {
            Symbols & s{Symbols_()};
            return s.names[id_];
        }

        bool operator == (Symbol const & other) const {
            return id_ == other.id_;
        }

        bool operator != (Symbol const & other) const {
            return id_ != other.id_;
        }

        size_t id() const {
            return id_;
        }

    private:

        size_t id_;

        struct Symbols {
            std::vector<std::string> names;
            std::unordered_map<std::string, size_t> lookup;
        };

        static Symbols & Symbols_() {
            static Symbols singleton;
            return singleton;
        }

        friend std::ostream & operator << (std::ostream & s, Symbol sym) {
            s << "`" << sym.name() << "`";
            return s;
        }

    }; // tiny::Symbol

} // namespace tiny

namespace std {

    template<>
    struct hash<tiny::Symbol> {
        size_t operator () (tiny::Symbol symbol) const {
            return std::hash<size_t>{}(symbol.id());
        }
    };

}