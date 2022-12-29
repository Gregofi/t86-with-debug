#pragma once

#include <string>
#include <cassert>
#include <unordered_map>

#include "symbol.h"
#include "helpers.h"
#include "ast.h"

namespace tiny {

    /** Base class for all types in tinyverse. 
     */
    class Type {
    public:

        class POD;

        class Pointer;

        class Struct;

        class FunPtr;

        class Alias;

        class Function;

        virtual ~Type() = default;

        /** Converts the type to a printable string.
         */
        std::string toString() const {
            std::stringstream ss;
            toStream(ss);
            return ss.str();
        }

        /** Each subtype should override it's specific method 
         *  (ie. Pointer should override `getPointer` and return true).
         */
        virtual bool isPOD() const { return false; }

        virtual bool isPointer() const { return false; };

        virtual bool isStruct() const { return false; };

        virtual bool isFunctionPointer() const { return false; }

        virtual bool isFunction() const { return false; }

        virtual size_t size() const = 0;

        virtual bool convertsToBoolean() const { return false; }

        /** Returns whether the type is fully defined. 
         */
        virtual bool isFullyDefined() const { return true; }

        static Type::POD* voidType() {
            return voidType_;
        }

        static Type::POD* intType() {
            return intType_;
        }

        static Type::POD* charType() {
            return charType_;
        }

        static Type::POD* doubleType() {
            return doubleType_;
        }

        static Type::Struct* getOrCreateStructType(Symbol name);

        static Type::Pointer* getOrCreatePointerType(Type* base);

        /**
         * Functions have unique names in tinyC, so use that as identifier for the type.
         * @param retType
         * @param name
         * @return
         */
        static Type::Function* getOrCreateFunctionType(Type* retType, const std::string& name);

        /**
         * Returns type based on the name, if no type exists then
         * returns nullptr.
         */
        static Type* getType(const std::string& name) {
            auto it = types().find(name);
            if (it == types().end()) {
                return nullptr;
            }
            return it->second;
        }

    private:
        virtual void toStream(std::ostream& s) const = 0;

        static std::unordered_map<std::string, Type*> initialize_types();

        static std::unordered_map<std::string, Type*>& types();

        static POD* voidType_;
        static POD* intType_;
        static POD* charType_;
        static POD* doubleType_;

    }; // tiny::Type

    /**
     * Plain old data type - ints, nulls, chars and so on.
     */
    class Type::POD: public Type {
    public:
        POD(Symbol name) :
                name_{name} {
            assert(name_ == Symbol::KwInt || name_ == Symbol::KwChar
                   || name_ == Symbol::KwDouble || name_ == Symbol::KwVoid);
        }

        Symbol name() const { return name_; }

        size_t size() const override {
            if (name_ == Symbol::KwChar) {
                return 1;
            } else if (name_ == Symbol::KwDouble) {
                return 8;
            } else if (name_ == Symbol::KwInt) {
                return 8;
            } else if (name_ == Symbol::KwVoid) {
                throw std::runtime_error("Void doesn't have size!");
            }
            throw std::runtime_error("Unknown type in 'POD::size()' function.");
        }

        bool isPOD() const override { return true; };

        bool convertsToBoolean() const override {
            return true;
        }

    private:

        void toStream(std::ostream& s) const override {
            s << name_;
        }

        Symbol name_;
    };

    class Type::Pointer: public Type {
    public:

        Pointer(Type* base) :
                base_{base} {
        }

        size_t size() const override { return 8; }

        bool isPointer() const override { return true; };
    private:
        void toStream(std::ostream& s) const override {
            s << "*";
            base_->toStream(s);
        }

        Type const* base_;
    };

    class Type::Struct: public Type {
    public:
        Struct(Symbol name) :
                name_{name} {
        }

        size_t size() const override {
            NOT_IMPLEMENTED;
        }

        bool isStruct() const override { return true; };

        bool isFullyDefined() const override { return defined_; }

        void markFullyDefined() { defined_ = true; }

        void addField(Symbol name, Type* type, ASTBase* ast);

    private:

        void toStream(std::ostream& s) const override {
            s << name_;
        }

        Symbol name_;
        bool defined_ = false;
        std::vector<std::pair<Symbol, Type*>> fields_;
    };

    class Type::FunPtr: public Type {
        Type* retType;
        std::vector<Type*> args;

        bool isFunctionPointer() const override { return true; };
    public:
        Type* getRetType() const { return retType; }

        const std::vector<Type*>& getArgs() const { return args; }
    };

    class Type::Function: public Type {
        Type* retType;
        std::vector<Type*> params;
    public:
        explicit Function(Type* retType) : retType(retType) {};

        void addArgument(Type* type) {
            params.emplace_back(type);
        }

        size_t size() const override {
            UNREACHABLE;
        }

        bool isFunction() const override { return false; }

        size_t paramCount() const {
            return params.size();
        }

        Type* argType(size_t i) const {
            return params[i];
        }

        Type* getRetType() const {
            return retType;
        }

    private:
        void toStream(std::ostream& s) const override {
            retType->toStream(s);
            s << " function(";
            for (const auto& p: params) {
                p->toStream(s);
                s << ",";
            }
            s << ")";
        }
    };
} // namespace tiny