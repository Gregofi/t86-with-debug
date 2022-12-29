#pragma once

#include "ast.h"
#include "common/helpers.h"
#include "common/types.h"
#include "common/environment.h"
#include <iterator>

namespace tinyc {

    using Type = tiny::Type;

    class Typechecker: public ASTVisitor {
        Environment<Type*> envs;
        std::map<std::string, Type*> functions;

        /// Check if 'ast' is convertible to bool, if not throws an error.
        void bool_or_throw(AST* ast) {
            if (!visitChild(ast)->convertsToBoolean()) {
                throw ParserError{STR("Condition must convert to bool, but '" << ast->type()->toString() << "' found"),
                                  ast->location()};
            }
        }

    public:

        void visit(AST* ast) override {
            visitChild(ast);
        }

        void visit([[maybe_unused]] ASTString* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit(ASTIdentifier* ast) override {
            auto id = envs.get(ast->name.name());
            // If variable doesn't exist then try to find function with the name.
            if (!id) {
                if (functions.contains(ast->name.name())) {
                    ast->setType(functions[ast->name.name()]);
                }
            } else {
                assert(id.value() != nullptr);
                ast->setType(*id);
            }
        }

        void visit([[maybe_unused]] ASTType* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTMember* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTMemberPtr* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTUnaryPostOp* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit(ASTInteger* ast) override {
            ast->setType(Type::intType());
        }

        void visit(ASTPointerType* ast) override {
            auto pointee_type = visitChild(ast->base);
            ast->setType(Type::getOrCreatePointerType(pointee_type));
        }

        void visit([[maybe_unused]] ASTArrayType* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit(ASTNamedType* ast) override {
            ast->setType(Type::getType(ast->name.name()));
        }

        void visit(ASTBreak* ast) override {
            ast->setType(Type::voidType());
        }

        void visit(ASTContinue* ast) override {
            ast->setType(Type::voidType());
        }

        void visit(ASTReturn* ast) override {
            // TODO: Check if return expr type is the same as function return type;
            visitChild(ast->value);
            ast->setType(Type::voidType());
        }

        void visit([[maybe_unused]] ASTIndex* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTVarDecl* ast) override {
            assert(ast != nullptr);
            // if (ast->value != nullptr) {
            //     visitChild(ast->value);
            // }
            // TODO: Ugly
            auto s = ast->varType->toString();
            if (s == "int") {
                ast->setType(Type::intType());
            } else if (s == "int*") {
                ast->setType(new Type::Pointer(Type::intType()));
            } else {
                UNREACHABLE;
            }
            envs.add(ast->name->name.name(), ast->type());
        }

        void visit([[maybe_unused]] ASTFunPtrDecl* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTAssignment* ast) override {
            visitChild(ast->value);
            ast->setType(visitChild(ast->lvalue));
        }

        void visit([[maybe_unused]] ASTAddress* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit(ASTCall* ast) override {
            Type* type = visitChild(ast->function);
            auto* fun = dynamic_cast<Type::Function*>(type);
            if (!fun) {
                throw ParserError(STR("Can only call functions, received type:" << type->toString()), ast->location());
            }
            ast->setType(fun->getRetType());
        }

        void visit([[maybe_unused]] ASTCast* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTWrite* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTRead* ast) override {
            NOT_IMPLEMENTED;
        }


        // Not, Inc, Dec, +, -,
        void visit(ASTUnaryOp* ast) override {
            Type* t = visitChild(ast->arg);
            if (ast->op == Symbol::Add || ast->op == Symbol::Sub) {
                // TODO overim si jestli to je ok typ a kdyz jo tak:
                ast->setType(t);
            } else if (ast->op == Symbol::Not) {
                if (t->convertsToBoolean()) {
                    ast->setType(Type::intType());
                }
            } else {
                NOT_IMPLEMENTED;
            }
        }

        void visit(ASTIf* ast) override {
            bool_or_throw(ast->cond.get());
            visitChild(ast->trueCase);
            if (ast->falseCase != nullptr) {
                visitChild(ast->falseCase);
            }
            ast->setType(Type::voidType());
        }

        void visit(ASTStructDecl* ast) override {
            Type::Struct* t = Type::getOrCreateStructType(ast->name);
            if (t == nullptr)
                throw ParserError{STR("Type " << ast->name << " already defined and not a struct"), ast->location()};
            if (ast->isDefinition) {
                if (t->isFullyDefined())
                    throw ParserError{STR("Struct " << ast->name << " already fully defined"), ast->location()};
                for (auto& i: ast->fields) {
                    Type* fieldType = visitChild(i.second);
                    t->addField(i.first->name, fieldType, i.second.get());
                }
            }
            t->markFullyDefined();
            ast->setType(t);
        }

        void visit(ASTFunDecl* ast) override {
            envs.push_env();

            Type* retType = visitChild(ast->typeDecl.get());
            Type::Function* fun = Type::getOrCreateFunctionType(retType, ast->name.name());

            // Save argument types
            for (auto& [typeAST, identifier]: ast->args) {
                Type* type = visitChild(typeAST);
                assert(type != nullptr);
                fun->addArgument(type);
                envs.add(identifier->name.name(), type);
            }

            ast->setType(fun);

            // TODO: Save return type to env.
            functions.emplace(ast->name.name(), ast->type());
            visitChild(ast->body);
            envs.pop_env();
        }

        void visit(ASTDouble* ast) override {
            ast->setType(Type::doubleType());
        }

        void visit(ASTChar* ast) override {
            ast->setType(Type::charType());
        }

        Type* arith_binary(const Type* left, const Type* right, bool dbl = false) {
#define DOUBLE_PROMOTION(l, r) ((l) == Type::doubleType() && ((r) == Type::doubleType() || (r) == Type::intType() || (r) == Type::charType()))
#define INT_PROMOTION(l, r) ((l) == Type::intType() && ((r) == Type::intType() || (r) == Type::charType()))
#define CHAR_PROMOTION(l, r) ((l) == Type::charType() && (r) == Type::charType())
            if (DOUBLE_PROMOTION(left, right) || DOUBLE_PROMOTION(right, left)) {
                return Type::doubleType();
            } else if (INT_PROMOTION(left, right) || INT_PROMOTION(right, left)) {
                return Type::intType();
            } else if (CHAR_PROMOTION(left, right) || CHAR_PROMOTION(right, left)) {
                return Type::charType();
            }
            return nullptr;
#undef DOUBLE_PROMOTION
#undef INT_PROMOTION
#undef CHAR_PROMOTION
        }

        void visit(ASTBinaryOp* ast) override {
            Type* left = visitChild(ast->left);
            Type* right = visitChild(ast->right);

            arith_binary(left, right);
            // Add and sub can be for integers and pointers
            if (ast->op == Symbol::Add || ast->op == Symbol::Sub) {
                if (left->isPointer()) {
                    ast->setType(ast->left->type());
                } else if (right != nullptr && right->isPointer()) {
                    ast->setType(ast->right->type());
                } else {
                    Type* type = arith_binary(left, right);
                    if (!type) {
                        throw ParserError(
                                STR("Can't perform binary operation '" << ast->op << "' with operands of type '"
                                                                       << left << "' and '" << right << "'"),
                                ast->location());
                    }
                    ast->setType(type);
                }
            } else if (ast->op == Symbol::Mul || ast->op == Symbol::Div) {
                Type* type = arith_binary(left, right);
                if (!type) {
                    throw ParserError(STR("Can't perform binary operation '" << ast->op << "' with operands of type '"
                                                                             << left << "' and '" << right << "'"),
                                      ast->location());
                }
                ast->setType(type);
            } else if (ast->op == Symbol::ShiftLeft || ast->op == Symbol::ShiftRight || ast->op == Symbol::BitAnd ||
                       ast->op == Symbol::BitOr) {
                if (left != right || (left != Type::charType() || left != Type::intType())) {
                    throw ParserError(STR("Can't perform binary operation '" << ast->op << "' with operands of type '"
                                                                             << left << "' and '" << right << "'"),
                                      ast->location());
                }
                ast->setType(left);
            } else if (ast->op == Symbol::And || ast->op == Symbol::Or || ast->op == Symbol::Xor) {
                if (!left->convertsToBoolean() || !right->convertsToBoolean()) {
                    throw ParserError(STR("Can't convert types '" << left << "' and '" << right << "' to boolean."),
                                      ast->location());
                }
                ast->setType(Type::intType());
            } else if (ast->op == Symbol::Lt || ast->op == Symbol::Gt || ast->op == Symbol::Gte ||
                       ast->op == Symbol::Lte || ast->op == Symbol::Eq || ast->op == Symbol::NEq) {
                if (left != right || (!left->isPointer() && left != Type::charType() && left != Type::intType() &&
                                      left != Type::doubleType())) {
                    throw ParserError(
                            STR("Can't compare types '" << left << "' and '" << right << "' to boolean with op '"
                                                        << ast->op << "'."), ast->location());
                }
                ast->setType(Type::intType());
            } else {
                throw ParserError("Unknown case", ast->location());
            }
        }

        void visit(ASTWhile* ast) override {
            bool_or_throw(ast->cond.get());

            visitChild(ast->body);
        }

        void visit(ASTDoWhile* ast) override {
            bool_or_throw(ast->cond.get());

            visitChild(ast->body);
        }

        void visit(ASTFor* ast) override {
            bool_or_throw(ast->cond.get());

            visitChild(ast->body);
        }

        void visit(ASTSwitch* ast) override {
            bool_or_throw(ast->cond.get());

            std::for_each(ast->cases.begin(), ast->cases.end(),
                          [this](const std::pair<int, std::unique_ptr<AST>>& ast) { return visitChild(ast.second); });
        }

        void visit(ASTBlock* ast) override {
            std::for_each(ast->body.begin(), ast->body.end(), [this](const std::unique_ptr<AST>& ast) {
                visitChild(ast);
            });
            ast->setType(Type::voidType());
        }

        void visit(ASTSequence* ast) override {
            Type* type = Type::voidType();
            for (auto& x: ast->body) {
                type = visitChild(x);
            }
            ast->setType(type);
        }

        void visit(ASTDeref* ast) override {
            Type* t = visitChild(ast->target);
            if (!t->isPointer()) {
                throw ParserError(STR("Can't dereference '" << t << "' type."), ast->location());
            }
            ast->setType(t);
        }

        Type* visitChild(AST* ast) {
            ASTVisitor::visitChild(ast);
            return ast->type();
        }

        template<typename T>
        Type* visitChild(std::unique_ptr<T> const& ptr) {
            return visitChild(ptr.get());
        }

    }; // Typechecker
}
