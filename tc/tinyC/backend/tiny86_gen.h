#pragma once

#include <fmt/core.h>
#include <map>
#include <memory>
#include <queue>
#include <optional>
#include <ranges>

#include "common/helpers.h"
#include "tinyC/backend/ir_visitor.h"
#include "tinyC/backend/instruction.h"
#include "tinyC/backend/context.h"
#include "tinyC/backend/function.h"
#include "tinyC/backend/basic_block.h"
#include "common/interval.h"
#include "common/logger.h"
#include "register_allocator.h"
#include "tinyC/backend/opcode.h"
#include "tinyC/backend/operand.h"

namespace tinyc {

#define INS_OP(op)                                                                                                     \
do {                                                                                                                   \
    program.add_ins(T86Ins::Opcode::MOV, reg_alloc->getRegister(ins.getID()), reg_alloc->getRegister(ins.getLeft()->getID())); \
    program.add_ins(T86Ins::Opcode::op, reg_alloc->getRegister(ins.getID()), reg_alloc->getRegister(ins.getRight()->getID())); \
} while(false)

    using namespace tiny;

    /**
     * Returns a map that for each instruction tells where it's been declared (essentially it's ID) and
     * ID of an instruction where it has been used for the last time.
     */
    static std::map<size_t, Interval> calculateRanges(const Function& f) {
        std::map<size_t, Interval> ranges;
        for (const auto& bb: f) {
            for (const auto& ins: *bb) {
                for (const auto& id: ins->getIdsOfChildren()) {
                    auto it = ranges.find(id);
                    if (it != ranges.end()) {
                        it->second.getEnd() = ins->getID();
                    } else {
                        ranges.emplace(id, Interval{ins->getID(), ins->getID()});
                    }
                }
            }
        }
        return ranges;
    }

    /**
     * Used for generating one body of a function, keeps necessary info about it.
     */
    class Tiny86FunctionGen: public IRVisitor {
        std::unique_ptr<AbstractRegisterAllocator> reg_alloc;
        Program& program;

        int allocated_space = 0;
        std::map<size_t, int> allocated;
        std::map<size_t, size_t>& functions;

        std::queue<std::pair<BasicBlock*, std::optional<size_t>>> bb_worklist;
        std::map<BasicBlock*, size_t> bb_finished;

        /**
         * Labels that jumps to the end of current functions.
         */
        std::set<size_t> end_jumps;
        
    public:
        explicit Tiny86FunctionGen(std::map<size_t, size_t>& functions,
                                   std::unique_ptr<AbstractRegisterAllocator> reg_alloc,
                                   Program& program)
                : reg_alloc(std::move(reg_alloc)), functions(functions), program(program) {

        }

        /**
         * Generates basic block code and returns label that is the first generated instruction in this block.
         * @param bb
         * @return
         */
        size_t generateBasicBlock(BasicBlock* bb) {
            // FIXME: Uses workaround with adding nop as first ins.
            auto begin = program.add_ins(T86Ins::Opcode::NOP);
            for (const auto& ins: *bb) {
                ins->accept(*this);
            }
            return begin;
        }

        void visit(const InsAdd& ins) override {
            INS_OP(ADD);
        };

        void visit(const InsSub& ins) override {
            INS_OP(SUB);
        }

        void visit(const InsMul& ins) override {
            INS_OP(MUL);
        }

        void visit(const InsMod& ins) override {
            INS_OP(MOD);
        }

        void visit(const InsAnd& ins) override {
            INS_OP(AND);
        }

        void visit(const InsOr& ins) override {
            INS_OP(OR);
        }

        void visit(const InsXor& ins) override {
            INS_OP(XOR);
        }

        void visit(const InsAlloca& ins) override {
            allocated[ins.getID()] = -(allocated_space + 1); // One for the saved EBP on stack
            allocated_space += 1;
            program.add_ins(T86Ins::Opcode::SUB, Register{Register::Type::SP}, Immediate{1});
        }

        void visit(const InsStore& ins) override {
            auto varOffset = MemoryRegisterOffset(Register(Register::Type::BP),
                                                 allocated.at(ins.getWhere()->getID()));
            program.add_ins(T86Ins::Opcode::MOV, varOffset, reg_alloc->getRegister(ins.getWhat()->getID()));
        }

        void visit(const InsLoad& ins) override {
            auto varOffset = MemoryRegisterOffset(Register(Register::Type::BP),
                                                 allocated.at(ins.getWhere()->getID()));
            program.add_ins(T86Ins::Opcode::MOV, reg_alloc->getRegister(ins.getID()), varOffset);
        }

        void visit(const InsRet& ins) override {
            program.add_ins(T86Ins::Opcode::MOV, reg_alloc->getAX(), reg_alloc->getRegister(ins.getVal()->getID()));

            // Ret
            end_jumps.emplace(program.add_ins(T86Ins::Opcode::JMP, Label{}));
        }

        void visit(const InsCmp& ins) override {
            // t86 (and x86 by extension) does not have comparison ins directly.
            // It however has various jumps. First, cmp instruction is created and
            // then jump based on the operator. So
            // %x = cmp == %y %z
            // is translated as
            //  cmp x, y
            //  je true
            // # false case
            //  mv z, 0
            //  jmp merge
            // true:
            //  mv z, 1
            // merge:
            //   ...
            program.add_ins(T86Ins::Opcode::CMP, reg_alloc->getRegister(ins.getLeft()->getID()), reg_alloc->getRegister(ins.getRight()->getID()));
            size_t true_b;

            switch (ins.getOp()) {
                case InsCmp::compare_op::EQ:
                    true_b = program.add_ins(T86Ins::Opcode::JE, Label());
                    break;
                case InsCmp::compare_op::NEQ:
                    true_b = program.add_ins(T86Ins::Opcode::JNE, Label());
                    break;
                case InsCmp::compare_op::LE:
                    true_b = program.add_ins(T86Ins::Opcode::JL, Label());
                    break;
                case InsCmp::compare_op::GE:
                    true_b = program.add_ins(T86Ins::Opcode::JG, Label());
                    break;
                case InsCmp::compare_op::GEQ:
                    true_b = program.add_ins(T86Ins::Opcode::JGE, Label());
                    break;
                case InsCmp::compare_op::LEQ:
                    true_b = program.add_ins(T86Ins::Opcode::JLE, Label());
                    break;
                default:
                    UNREACHABLE;
            }

            // False case is fall through
            program.add_ins(T86Ins::Opcode::MOV, reg_alloc->getRegister(ins.getID()), 0);
            auto false_case_jmp = program.add_ins(T86Ins::Opcode::JMP, Label{});

            // True case
            auto true_case_begin = program.add_ins(T86Ins::Opcode::MOV, reg_alloc->getRegister(ins.getID()), 1);
            // Since it is a visitor, we have no good way of tracking next instruction, so we create a nop to jump to as
            // the 'merge' block
            auto merge = program.add_ins(T86Ins::Opcode::NOP);

            // Patch up the jumps
            program.patch(true_b, true_case_begin);
            program.patch(false_case_jmp, merge);
        }

        size_t generateFunction(const Function& fun) {
            allocated_space = 0;
            allocated.clear();

            // Function prolog
            // PUSH EBP
            // MOV EBP, ESP
            // Save the returned label of the first instruction in the function
            auto fun_begin = program.add_ins(T86Ins::Opcode::PUSH, Register(Register::Type::BP));
            functions.emplace(fun.getID(), fun_begin);
            program.add_ins(T86Ins::Opcode::MOV, Register(Register::Type::BP), Register(Register::Type::SP));

            // Keep track of arguments
            int offset = 2; // There is already return address and EBP on stack

            // Because of used call conventions we have to pop the arguments in opposite order
            // TODO: Use reverse view in C++20
            for (auto it = fun.getArgs().rbegin(); it != fun.getArgs().rend(); ++ it) {
                program.add_ins(T86Ins::Opcode::MOV, reg_alloc->getRegister((*it)->getID()), MemoryRegisterOffset{Register(Register::Type::BP), offset});
                offset += 1;
            }

            // Body
            assert(!fun.empty());
            bb_worklist.emplace(fun.front(), std::nullopt);

            // Essentially BFS over basic blocks.
            // Inserting already finished basic blocks is permitted,
            // they will not be generated, but their label
            // will be patched.
            while (!bb_worklist.empty()) {
                auto [bb, to_patch] = bb_worklist.front();
                bb_worklist.pop();
                if (!bb_finished.contains(bb)) {
                    auto label = generateBasicBlock(bb);
                    bb_finished.emplace(bb, label);
                }
                // Finished now always contains the new basic block
                if (to_patch) {
                    program.patch(*to_patch, bb_finished.at(bb));
                }
            }

            // Update all jumps that jump to the function end
            // ALL THESE JUMPS MUST SET THE AX REGISTER FOR RETURN VALUE BEFOREHAND!
            size_t end = program.add_ins(T86Ins::Opcode::NOP);
            for (const auto& label: end_jumps) {
                program.patch(label, end);
            }

            // Epilog
            if (allocated_space > 0) {
                program.add_ins(T86Ins::Opcode::ADD, Register(Register::Type::SP), allocated_space);
            }
            program.add_ins(T86Ins::Opcode::POP, Register(Register::Type::BP));

            // Ret
            program.add_ins(T86Ins::Opcode::RET);

            return fun_begin;
        }

        void visit([[maybe_unused]] const Function& fun) override {
            UNREACHABLE;
        }

        void visit(const BasicBlock& bb) override {
            for (const auto& ins: bb) {
                ins->accept(*this);
            }
        }

        /**
         * Pushes the arguments of ins onto the stack and generates call to procedure
         */
        void visit(const InsCall& ins) override {
            // registersInUse is not O(1) and they can change in this function
            auto regs_in_use = reg_alloc->registersInUse();
            // Save registers in use to stack
            for (const auto& reg: regs_in_use) {
                program.add_ins(T86Ins::Opcode::PUSH, reg);
            }
            // Push arguments on stack
            for (const auto& i: ins.getArgs()) {
                // Careful, we don't want to save the argument register
                program.add_ins(T86Ins::Opcode::PUSH, reg_alloc->getRegister(i->getID()));
            }
            program.add_ins(T86Ins::Opcode::CALL, functions.at(ins.getVal()->getID()));
            // Clean arguments from stack
            program.add_ins(T86Ins::Opcode::ADD, Register(Register::Type::SP), static_cast<int64_t>(ins.getArgs().size()));
            // Retrieve saved registers
            for (auto it = regs_in_use.rbegin(); it != regs_in_use.rend(); ++ it) {
                program.add_ins(T86Ins::Opcode::POP, *it);
            }
            program.add_ins(T86Ins::Opcode::MOV, reg_alloc->getRegister(ins.getID()), reg_alloc->getAX());
        }

        void visit(const InsLoadImm& ins) override {
            program.push_back(ins.getConstant()->generateMov(reg_alloc->getRegister(ins.getID())));
        }

        void visit([[maybe_unused]] const Instruction& ins) override {
            UNREACHABLE;
        }

        void visit([[maybe_unused]] const FunctionArgument& ins) override {
            NOT_IMPLEMENTED;
        }

        void visit(const InsJmp& ins) override {
            auto jmp = program.add_ins(T86Ins::Opcode::JMP, Label{});
            bb_worklist.emplace(ins.getDest(), jmp);
        }

        void visit(const InsCondJmp& ins) override {
            // Condition always stores 0 or 1 in the ID register
            program.add_ins(T86Ins::Opcode::CMP, reg_alloc->getRegister(ins.getCond()->getID()), 0);
            auto false_jmp = program.add_ins(T86Ins::Opcode::JZ, Label{});
            auto true_jmp = program.add_ins(T86Ins::Opcode::JNZ, Label{});

            bb_worklist.emplace(ins.getTrueBb(), true_jmp);
            bb_worklist.emplace(ins.getFalseBb(), false_jmp);
        }

        void visit(const InsIndirectLoad& ins) override {
            auto varOffset = MemoryRegisterOffset(Register(Register::Type::BP),
                                        allocated.at(ins.getLoadVal()->getID()));
            program.add_ins(T86Ins::Opcode::LEA, reg_alloc->getRegister(ins.getID()), varOffset);
        }

        void visit(const InsIndirectStore& ins) override {
            auto varOffset = MemoryRegisterOffset(Register(Register::Type::BP), allocated.at(ins.getWhere()->getID()));
            program.add_ins(T86Ins::Opcode::MOV, reg_alloc->getRegister(ins.getID()), varOffset);
            program.add_ins(T86Ins::Opcode::MOV, MemoryRegister{reg_alloc->getRegister(ins.getID())}, reg_alloc->getRegister(ins.getWhat()->getID()));
        }

        void visit(const InsLShift& ins) override {
            INS_OP(LSH);
        }
    };

    class Tiny86Gen {
        Program program;
        std::map<size_t, size_t> functions;

        static constexpr int REGISTERS = 5;
    public:

        Program generate(Context& context, bool exit_print) {
            // Generate program Entry Point
            size_t call = program.add_ins(T86Ins::Opcode::CALL, Label{});
            if (exit_print) {
                program.add_ins(T86Ins::Opcode::PUTNUM, Register{REGISTERS});
            }
            program.add_ins(T86Ins::Opcode::HALT);

            for (const auto& [name, global]: context) {
                auto* f = dynamic_cast<Function*>(global);
                // FIXME: We can only generate functions, globals would case crash.
                if (f == nullptr) {
                    throw std::runtime_error("Globals are not supported");
                }
                std::map ranges = calculateRanges(*f);
                auto alloc = std::make_unique<LinearRegisterAllocator>(std::move(ranges), program, REGISTERS);
                Tiny86FunctionGen g(functions, std::move(alloc), program);
                g.generateFunction(*f);

                if (name == "main") {
                    program.patch(call, functions.at(global->getID()));
                }
            }
            return program;
        }
    };
}
