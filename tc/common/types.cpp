#include "types.h"

namespace tiny {

    Type::POD * Type::voidType_;
    Type::POD * Type::intType_;
    Type::POD * Type::charType_;
    Type::POD * Type::doubleType_;

    Type::Struct * Type::getOrCreateStructType(Symbol name) {
        auto t = types();
        auto i = t.find(name.name());
        if (i == t.end())
            i = t.insert(std::make_pair(name.name(), new Type::Struct{name})).first;
        return dynamic_cast<Struct*>(i->second);
    }

    Type::Pointer* Type::getOrCreatePointerType(Type* base) {
        std::string name = base->toString() + "*";
        auto& tps = types();
        auto it = tps.find(base->toString());

        if (!tps.contains(name)) {
            auto t = new Type::Pointer(base);
            tps.insert({name, t});
            return t;
        }
        return dynamic_cast<Type::Pointer*>(tps.at(name));
    }

    Type::Function* Type::getOrCreateFunctionType(Type* retType, const std::string &name) {

        auto& tps = types();

        if (!tps.contains(name)) {
            auto t = new Type::Function(retType);
            tps.emplace(name, t);
            return t;
        }
        return dynamic_cast<Type::Function*>(tps.at(name));
    }

    std::unordered_map<std::string, Type *> Type::initialize_types() {
        std::unordered_map<std::string, Type *> result;
        voidType_ = new POD{Symbol::KwVoid};
        result.insert(std::make_pair("void", voidType_));
        intType_ = new POD{Symbol::KwInt};
        result.insert(std::make_pair("int", intType_));
        doubleType_ = new POD{Symbol::KwDouble};
        result.insert(std::make_pair("double", doubleType_));
        charType_ = new POD{Symbol::KwChar};
        result.insert(std::make_pair("char", charType_));

        return result;
    }

    std::unordered_map<std::string, Type*> & Type::types() {
        static std::unordered_map<std::string, Type*> types = initialize_types();
        return types;
    }

    void Type::Struct::addField(Symbol name, Type * type, ASTBase * ast) {
        if (! type->isFullyDefined())
            throw ParserError{STR("Field " << name.name() << " has not fully defined type " << type->toString()), ast->location()};
        for (auto & f : fields_)
            if (f.first == name)
                throw ParserError{STR("Field " << name.name() << " already defined "), ast->location()};
        fields_.push_back(std::make_pair(name, type));
    }

}