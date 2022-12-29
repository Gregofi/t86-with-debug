#pragma once

#include "common/helpers.h"
#include "tinyC/ast.h"
#include "tinyC/backend/value.h"
#include "tinyC/backend/ir_builder.h"
#include "tinyC/backend/context.h"
#include "tinyC/backend/constant.h"
#include "tinyC/backend/common.h"
#include "tinyC/backend/pointers.h"
#include <stdexcept>
#include <map>
#include <string>

namespace tinyc {

    /**
      * Generates IR from AST.
      *
      */
    class Codegen: public ASTVisitor {
        struct Environment {
            std::map<std::string, Value*> locals;
        };

#define CG_VISIT(ast) (ASTVisitor::visitChild((ast)), ret)
#define CG_RETURN(val) do {(ret = (val)); return;} while(false)
        Value* ret;
        IRBuilder builder;
        Context* context;
        std::vector<Environment> envs;

        // For every Value*, it maps another Values which use this one
        std::map<Value*, std::vector<Value*>> usages;
    public:
        explicit Codegen(Context* context) : context(context) {}

        void visit(AST* ast) override {
            CG_RETURN(CG_VISIT(ast));
        }

        void visit(ASTInteger* ast) override {
            auto* loadimm = builder.createILoadImm(ConstantInt::get(ast->value));
            CG_RETURN(loadimm);
        }

        void visit(ASTDouble* ast) override {
            CG_RETURN(builder.allocateType(ast->type()));
        }

        void visit(ASTChar* ast) override {
            CG_RETURN(builder.allocateType(ast->type()));
        }

        void visit([[maybe_unused]] ASTString* ast) override {
            NOT_IMPLEMENTED;
        }

        Value* findInEnvs(const std::string& id) const {
            auto it = envs.rbegin();
            for (;it != envs.rend(); ++ it) {
                if (it->locals.contains(id)) {
                    break;
                }
            }
            if (it == envs.rend()) {
                throw CodegenError(STR("Identifier " << id << " not found."));
            }
            return it->locals.at(id);
        }

        void visit(ASTIdentifier* ast) override {
            auto local = findInEnvs(ast->name.name());
            if (auto alloca = dynamic_cast<InsAlloca*>(local)) {
                local = builder.createILoad(local); // FIXME: unused alloca?
            }
            CG_RETURN(local);
        }

        void visit([[maybe_unused]] ASTType* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTPointerType* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTArrayType* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTNamedType* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTSequence* ast) override {
            // Seq should be a comma expr (1 + 2, 3, 4), but it is prolbably
            // buggy and behaves like a block statement
            Value* val = nullptr;
            for (const auto& block_ast: ast->body) {
                val = CG_VISIT(block_ast.get());
            }
            CG_RETURN(val);
        }

        void visit(ASTBlock* ast) override {
            push_environment();
            Value* val;
            for (const auto& block_ast: ast->body) {
                val = CG_VISIT(block_ast.get());
            }
            CG_RETURN(val);
        }

        void visit([[maybe_unused]] ASTVarDecl* ast) override {
            auto space = builder.allocateType(ast->type());
            if (ast->value != nullptr) {
                auto value = CG_VISIT(ast->value);
                builder.createIStore(value, space);
            }
            top_env().locals.emplace(ast->name->name.name(), space);
            CG_RETURN(space);
        }

        void visit(ASTFunDecl* ast) override {
            push_environment();

            /* Create body of the function */
            auto bb = new BasicBlock();
            builder.SetInsertionPoint(bb);

            /* Insert local vars to environment and map parameters to allocated values */
            std::vector<FunctionArgument*> params;
            for (const auto& [type, id]: ast->args) {
                // TODO : This machinery seems kinda unnecessary
                // First, create function parameter. Allocate a space for it and store it there,
                // so it is mutable.
                auto arg = new FunctionArgument(type->type());
                InsAlloca* space = builder.allocateType(type->type());
                InsStore* store = builder.createIStore(arg, space);
                top_env().locals.emplace(id->name.name(), space);
                params.emplace_back(arg);
            }
            /* Create the actual function type */
            auto fun = Function::CreateFunction(ast->name.name(), ast->type(), params, *context, ast->name.name(),
                                                ast->name.name() == "main");

            bb->setParent(fun);
            fun->addBB(bb);
            context->addFunction(ast->name.name(), fun);

            CG_VISIT(ast->body);
            pop_environment();
        }

        void visit([[maybe_unused]] ASTStructDecl* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTFunPtrDecl* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit(ASTIf* ast) override {
            auto* fun = builder.getInsertionPoint()->getParent();
            assert(fun != nullptr);

            auto* if_bb = new BasicBlock;
            auto* else_bb = new BasicBlock;
            auto* merge_bb = new BasicBlock;

            /* The if condition */
            auto* cond = CG_VISIT(ast->cond);
            builder.createICondJmp(cond, if_bb, ast->falseCase ? else_bb : merge_bb);

            /* The 'if' branch */
            builder.SetInsertionPoint(if_bb);
            CG_VISIT(ast->trueCase);
            builder.createIJmp(merge_bb);

            /* The 'else' branch */
            if (ast->falseCase) {
                builder.SetInsertionPoint(else_bb);
                CG_VISIT(ast->falseCase);
                builder.createIJmp(merge_bb);
                fun->addBB(else_bb);
            }
            builder.SetInsertionPoint(merge_bb);

            /* Add the code to the function */
            fun->addBB(if_bb);
            fun->addBB(merge_bb);
        }

        void visit([[maybe_unused]] ASTSwitch* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTWhile* ast) override {
            auto* fun = builder.getInsertionPoint()->getParent();
            assert(fun != nullptr);

            auto* while_bb = new BasicBlock;
            auto* cond_bb = new BasicBlock;
            auto* after_bb = new BasicBlock;
            
            builder.createIJmp(cond_bb);

            builder.SetInsertionPoint(cond_bb);
            auto* val_cond = CG_VISIT(ast->cond);
            builder.createICondJmp(val_cond, while_bb, after_bb);

            builder.SetInsertionPoint(while_bb);
            CG_VISIT(ast->body);
            builder.createIJmp(cond_bb);

            builder.SetInsertionPoint(after_bb);

            fun->addBB(while_bb);
            fun->addBB(cond_bb);
            fun->addBB(after_bb);

            CG_RETURN(while_bb);
        }

        void visit([[maybe_unused]] ASTDoWhile* ast) override {
            auto* fun = builder.getInsertionPoint()->getParent();
            assert(fun != nullptr);

            auto* while_bb = new BasicBlock;
            auto* after_bb = new BasicBlock;

            builder.createIJmp(while_bb);
            builder.SetInsertionPoint(while_bb);
            CG_VISIT(ast->body);
            auto* cond = CG_VISIT(ast->cond);
            builder.createICondJmp(cond, while_bb, after_bb);
            builder.SetInsertionPoint(after_bb);

            fun->addBB(while_bb);
            fun->addBB(after_bb);

            CG_RETURN(while_bb);
        }

        void visit([[maybe_unused]] ASTFor* ast) override {
            auto fun = builder.getInsertionPoint()->getParent();

            auto bb_cond = new BasicBlock;
            auto bb_body = new BasicBlock;
            auto bb_increment = new BasicBlock;
            auto bb_after = new BasicBlock;

            // Init
            CG_VISIT(ast->init);
            builder.createIJmp(bb_cond);

            // Condition
            builder.SetInsertionPoint(bb_cond);
            auto cond = CG_VISIT(ast->cond);
            builder.createICondJmp(cond, bb_body, bb_after);

            // Body
            builder.SetInsertionPoint(bb_body);
            CG_VISIT(ast->body);
            builder.createIJmp(bb_increment);

            // Increment
            builder.SetInsertionPoint(bb_increment);
            CG_VISIT(ast->increment);
            builder.createIJmp(bb_cond);

            // After
            builder.SetInsertionPoint(bb_after);

            fun->addBB(bb_cond);
            fun->addBB(bb_body);
            fun->addBB(bb_increment);
            fun->addBB(bb_after);

            CG_RETURN(bb_after);
        }

        void visit([[maybe_unused]] ASTBreak* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTContinue* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit(ASTReturn* ast) override {
            auto value = CG_VISIT(ast->value);
            CG_RETURN(builder.createIRet(value));
        }

        void visit(ASTBinaryOp* ast) override {
            // Load the values if they are stored in allocas.
            auto left = CG_VISIT(ast->left);
            auto right = CG_VISIT(ast->right);
            if (auto* alloca = dynamic_cast<InsAlloca*>(right)) {
                right = builder.createILoad(right);
            }

            if (ast->op == Symbol::Add) {
                CG_RETURN(builder.createIAdd(left, right));
            } else if (ast->op == Symbol::Sub) {
                CG_RETURN(builder.createISub(left, right));
            } else if (ast->op == Symbol::Mul) {
                CG_RETURN(builder.createIMul(left, right));
            } else if (ast->op == Symbol::Div) {
                CG_RETURN(builder.createIDiv(left, right));
            } else if (ast->op == Symbol::Eq) {
                CG_RETURN(builder.createICmp(InsCmp::compare_op::EQ, left, right));
            } else if (ast->op == Symbol::NEq) {
                CG_RETURN(builder.createICmp(InsCmp::compare_op::NEQ, left, right));
            } else if (ast->op == Symbol::Gt) {
                CG_RETURN(builder.createICmp(InsCmp::compare_op::GE, left, right));
            } else if (ast->op == Symbol::Gte) {
                CG_RETURN(builder.createICmp(InsCmp::compare_op::GEQ, left, right));
            } else if (ast->op == Symbol::Lte) {
                CG_RETURN(builder.createICmp(InsCmp::compare_op::LEQ, left, right));
            } else if (ast->op == Symbol::Lt) {
                CG_RETURN(builder.createICmp(InsCmp::compare_op::LE, left, right));
            } else {
                NOT_IMPLEMENTED;
            }
        }

        void visit([[maybe_unused]] ASTAssignment* ast) override {
            auto* expr = CG_VISIT(ast->value);

            if (auto* id = dynamic_cast<ASTIdentifier*>(ast->lvalue.get())) {
                auto* local = findInEnvs(id->name.name());
                auto* store = builder.createIStore(expr, local);
                CG_RETURN(store);
            } else if (auto* deref = dynamic_cast<ASTDeref*>(ast->lvalue.get())) {
                // FIXME: I can't hear you over the sound of dynamic casts
                auto* id = dynamic_cast<ASTIdentifier*>(deref->target.get());
                if (id == nullptr) {
                    throw CodegenError("Can only indirectly write to identifiers.");
                }
                auto* local = findInEnvs(id->name.name());;
                auto* store = builder.createIndirectStore(expr, local);
                CG_RETURN(store);
            }
            UNREACHABLE;
        }

        void visit([[maybe_unused]] ASTUnaryOp* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTUnaryPostOp* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTAddress* ast) override {
            // TODO: Check if it has address
            // TODO: Only work with identifiers
            auto* id = dynamic_cast<ASTIdentifier*>(ast->target.get());
            if (id == nullptr) {
                throw CodegenError("Address operator & can only be used on identifiers.");
            }

            auto* alloca = dynamic_cast<InsAlloca*>(findInEnvs(id->name.name()));
            if (alloca == nullptr) {
                throw CodegenError("Can't take address of non-alloca-ted memory");
            }
            CG_RETURN(builder.createIndirectLoad(alloca));
        }

        void visit([[maybe_unused]] ASTDeref* ast) override {
            auto* id = dynamic_cast<ASTIdentifier*>(ast->target.get());
            if (id == nullptr) {
                throw CodegenError("Only dereference of identifiers is supported.");
            }
            auto* val = CG_VISIT(id);
            CG_RETURN(builder.createIndirectLoad(val));
        }

        void visit([[maybe_unused]] ASTIndex* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTMember* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit([[maybe_unused]] ASTMemberPtr* ast) override {
            NOT_IMPLEMENTED;
        }

        void visit(ASTCall* ast) override {
            // TODO: Lets live in a pretty world where function calls can only be
            // performed on identifiers.
            auto* funAST = dynamic_cast<ASTIdentifier*>(ast->function.get());
            assert(funAST != nullptr);

            auto function = context->getFunction(funAST->name.name());
            assert(function != nullptr);
            // Evaluate arguments
            std::vector<Value*> args;
            std::transform(ast->args.begin(), ast->args.end(), std::back_inserter(args),
                           [this](const std::unique_ptr<AST>& ast) {
                               return CG_VISIT(ast.get());
                           });
            CG_RETURN(builder.createICall(function, args));
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

        void push_environment() {
            envs.emplace_back();
        }

        void pop_environment() {
            envs.pop_back();
        }

        Environment& top_env() {
            return envs.back();
        }


#undef CG_VISIT
#undef CG_RETURN
    };
}