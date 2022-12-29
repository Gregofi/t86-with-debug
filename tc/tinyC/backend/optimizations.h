#include <cmath>

#include "tinyC/backend/constant.h"
#include "tinyC/backend/function.h"
#include "tinyC/backend/instruction.h"
#include "tinyC/backend/context.h"
#include "tinyC/backend/ir_visitor.h"

namespace tinyc {
    class Optimization {
    public:
        virtual ~Optimization() = default;
        virtual void optimize(Context& ctx) = 0;
    };

    inline void replaceUsage(Function* fun, Value* new_val, Value* old_val) {
        for (auto& bb : *fun) {
            for (auto& ins : *bb) {
                ins->updateUsage(new_val, old_val);
            }
        }
    }

    /// Naive dead code removal (does not remove transitive use)
    /// It is aimed at removing dead code after other optimizations
    /// TODO: This is really fast hack, it should be remade,
    ///       may need to be modified for every new instruction.
    class DeadCodeRemoval : public Optimization {
    public:
        void optimize(Context& ctx) override {
            for (auto& [id, obj]: ctx) {
                auto* fun = dynamic_cast<Function*>(obj);
                if (fun == nullptr) {
                    continue;
                }

                std::set<std::size_t> used_vals;
                for (auto& bb: *fun) {
                    for (auto& ins: *bb) {
                        auto used = ins->getIdsOfChildren();
                        used.erase(ins->getID());

                        used_vals.insert(used.begin(), used.end());
                        if (dynamic_cast<InsRet*>(ins) != nullptr 
                            || dynamic_cast<InsStore*>(ins) != nullptr
                            || dynamic_cast<InsIndirectStore*>(ins) != nullptr
                            || dynamic_cast<InsCall*>(ins) != nullptr
                            || dynamic_cast<InsJmp*>(ins) != nullptr
                            || dynamic_cast<InsCondJmp*>(ins) != nullptr) {
                            used_vals.emplace(ins->getID());
                        }
                    }
                }

                for (auto& bb: *fun) {
                    for (auto it = bb->begin(); it != bb->end(); ) {
                        if (!used_vals.contains((*it)->getID())) {
                            it = bb->erase(it);
                        } else {
                            ++ it;
                        }
                    }
                }
            }
        }
    };

    /// Removes functions which are not called anywhere (main is never removed)
    class DeadCallsRemoval : public Optimization {
    public:
        void optimize(Context& ctx) override {
            std::set<size_t> used_funcs;

            // Save all IDS of function that are called + main
            for (auto& [id, obj]: ctx) {
                auto* fun = dynamic_cast<Function*>(obj);
                if (fun == nullptr) {
                    continue;
                }

                if (fun->isMain()) {
                    used_funcs.emplace(fun->getID());
                }

                for (auto& bb: *fun) {
                    for (auto& ins: *bb) {
                        auto call = dynamic_cast<InsCall*>(ins);
                        if (call == nullptr) {
                            continue;
                        }
                        used_funcs.emplace(call->getVal()->getID());
                    }
                }
            }

            // Remove function if it is not called
            for (auto it = ctx.begin(); it != ctx.end(); ) {
                if (!used_funcs.contains(it->second->getID())) {
                    it = ctx.erase(it);
                } else {
                    ++ it;
                }
            }
        }
    };

    class StrengthReduction : public Optimization {
        /// Taken from https://stackoverflow.com/questions/600293/how-to-check-if-a-number-is-a-power-of-2?answertab=trending#tab-top
        bool IsPowerOfTwo(int64_t x) {
            return (x != 0) && ((x & (x - 1)) == 0);
        }
    public:
        void optimize(Context& ctx) override {
            for (auto& [id, obj]: ctx) {
                auto* fun = dynamic_cast<Function*>(obj);
                if (fun == nullptr) {
                    continue;
                }

                for (auto& bb: *fun) {
                    for (size_t i = 0; i < bb->size(); ++ i) {
                        auto mul = dynamic_cast<InsMul*>((*bb)[i]);
                        if (mul == nullptr) {
                            continue;
                        }

                        if (auto constant = dynamic_cast<InsLoadImm*>(mul->getRight())) {
                            if (auto intConstant = dynamic_cast<ConstantInt*>(constant->getConstant()); intConstant != nullptr && IsPowerOfTwo(intConstant->getVal())) {
                                // Create new constant and save it to bb
                                Instruction* newconst = new InsLoadImm(ConstantInt::get(log2(intConstant->getVal())));
                                bb->insert(std::next(bb->begin(), i), newconst);

                                Instruction* newop = new InsLShift(mul->getLeft(), newconst);
                                // TODO: Instructions should remember who is using them
                                // Entire pass of whole IR should not be needed
                                (*bb)[i + 1] = newop;
                                replaceUsage(fun, newop, mul);
                            }
                        }
                    }
                }
            }
        }
    };
}