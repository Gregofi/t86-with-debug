#pragma once

#include <cassert>
#include <bitset>
#include <deque>
#include <ranges>
#include <set>
#include <map>
#include "common/interval.h"
#include "operand.h"
#include "opcode.h"

namespace tinyc {
    class AbstractRegisterAllocator {
    public:
        virtual ~AbstractRegisterAllocator() = default;

        virtual Register getRegister(size_t value_id) = 0;

        virtual Register getAX() const = 0;

        virtual std::set<Register> registersInUse() const = 0;
    };

    /**
     * Uses infinitely many registers (O(|IR instructions|))
     */
    class InfinityRegisterAllocator: public AbstractRegisterAllocator {
        std::set<int> used_registers;
    public:
        Register getRegister(size_t value_id) override;

        virtual Register getAX() const override {
            return Register{2 << 8};
        };

        std::set<Register> registersInUse() const override;
    };

/* ====================================================================== */

    class LinearRegisterAllocator: public AbstractRegisterAllocator {
        static const size_t MAX_MEMORY_AVAILABLE = 200;
        const size_t MAX_REGISTERS;

        using reg_id = size_t;
        using value_id = size_t;
        using memory_offset = size_t;

        /**
         * Manages spilled values and available memory
         */
        class SpilledManager {
            std::array<bool, MAX_MEMORY_AVAILABLE> taken_memory{};
            std::map<value_id, memory_offset> spilled;
            Program& program;
        public:
            explicit SpilledManager(Program& program) : program(program) {}

            memory_offset fetchFreeMemoryCell() const;

            memory_offset storeRegToMem(reg_id reg, value_id val);

            bool isSpilled(value_id val) const;

            /**
             * Stores value that is stored on val to given register.
             * The memory slot is also freed.
             */
            void storeMemToReg(value_id val, reg_id reg);
        };

        /**
         * Manages used and free registers
         */
        class RegisterManager {
            std::vector<bool> active_registers{};
            std::map<value_id, reg_id> register_mapping;
            std::deque<reg_id> register_usage;
            SpilledManager& memory_manager;
            Program& program;

            /**
             * Updates given register in the queue to be the last used register.
             * @param id - ID of the register
             */

        public:
            explicit RegisterManager(Program& program, SpilledManager& memory_manager, size_t registers_cnt)
                    : active_registers(registers_cnt, false), program(program), memory_manager(memory_manager) {}


            void updateRegisterUse(reg_id id);

            void matchValToReg(value_id val, reg_id reg);

            /**
             * Returns a register that is mapped to given value, or nullopt if the value is not mapped to any register
             * @param value
             * @return std::optional<reg_id>
             */
            std::optional<reg_id> getRegisterId(value_id value) const;

            /**
             * Returns set of registers that are currently in use.
             */
            std::set<Register> registersInUse() const;

            /**
             * Spills oldest register. This register is returned.
             * It is still considered as used register.
             */
            reg_id spillRegister();

            /**
             * Finds and returns first free register if available, otherwise returns nullopt.
             */
            std::optional<reg_id> fetchFreeRegister() const;
        };


        const std::map<value_id, Interval> ins_ranges;

        Program& program;

        SpilledManager spilled_manager;
        RegisterManager reg_manager;

    public:
      LinearRegisterAllocator(std::map<size_t, Interval> ins_ranges,
                              Program& program,
                              size_t reg_count = 4);

      Register getRegister(value_id value_id) override;

      Register getAX() const override;

      std::set<Register> registersInUse() const override;
    };
}


