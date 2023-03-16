#pragma once

#include "program.h"
#include "instruction.h"
#include "ram.h"
#include "cpu/register.h"
#include "cpu/reservation_station.h"
#include "cpu/register_allocation_table.h"
#include "cpu/branchpredictor.h"
#include "cpu/memory_writes_manager.h"

#include <vector>
#include <list>
#include <variant>
#include <cstddef>
#include <array>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <set>

namespace tiny::t86 {
    class Cpu {
    public:
        /// A singleton class
        class Config {
        public:
            static Config& instance() {
                static Config c;
                return c;
            }

            constexpr static const char* registerCountConfigString = "-registerCnt";

            constexpr static std::size_t defaultRegisterCount = 10;

            constexpr static const char* floatRegisterCountConfigString = "-floatRegisterCnt";

            constexpr static std::size_t defaultFloatRegisterCount = 5;

            constexpr static const char* aluCountConfigString = "-aluCnt";

            constexpr static std::size_t defaultAluCount = 1;

            constexpr static const char* reservationStationEntriesCountConfigString = "-reservationStationEntriesCnt";

            constexpr static std::size_t defaultReservationStationEntriesCount = 2;

            constexpr static const char* ramSizeConfigString = "-ram";

            constexpr static std::size_t defaultRamSize = 1024;

            constexpr static const char* ramGatesCountConfigString = "-ramGates";

            constexpr static std::size_t defaultRamGatesCount = 4;

            constexpr static const char* debuggerPortString = "-debuggerPort";

            std::size_t registerCnt() const;

            std::size_t floatRegisterCnt() const;

            std::size_t aluCnt() const;

            std::size_t reservationStationEntriesCnt() const;

            std::size_t ramSize() const;

            std::size_t ramGatesCount() const;

            std::size_t getExecutionLength(const Instruction* ins) const;

        private:
            Config();
        };

        // Max instruction operands - for example ADD R1 R2 has 3 (destination and 2 source)
        static constexpr std::size_t maxInstructionOperands = 3;

        // These are Pc, Sp, Bp and Flags
        static constexpr std::size_t specialRegistersCnt = 4;

        // This is how many renamed registers can be in use translate once for once instruction
        //  (it is quite generous, usually around 3 will be used translate once)
        static constexpr std::size_t possibleRenamedRegisterCnt = maxInstructionOperands + specialRegistersCnt;

        static constexpr int TRAP_FLAG_INTERRUPT = 1;

        static constexpr uint64_t TRAP_FLAG_MASK = 0x0400;

        Cpu();

        Cpu(size_t registerCount);

        Cpu(size_t registerCount, size_t floatRegisterCount);

        Cpu(std::size_t registerCount, std::size_t floatRegisterCount, size_t ramSize);

        Cpu(std::size_t registerCount, std::size_t floatRegisterCount, std::size_t aluCnt, std::size_t reservationStationEntriesCount, std::size_t ramSize, std::size_t ramGatesCnt);

        // These do not include special registers
        std::size_t registersCount() const {
            return registerCnt_;
        }

        std::size_t floatRegistersCount() const {
            return floatRegisterCnt_;
        }

        std::size_t physicalRegistersCount() const {
            return physicalRegisterCnt_;
        }

        void connectBreakHandler(std::function<void(Cpu&)> handler);

        void doBreak();

        void start(Program&& program);

        void tick();

        void jump(const ReservationStation::Entry& entry, bool taken);

        int64_t getRegister(PhysicalRegister reg) const;

        double getFloatRegister(PhysicalRegister reg) const;

        void setRegister(PhysicalRegister reg, int64_t value);

        void setRegister(PhysicalRegister reg, double value);

        MemoryWrite::Id currentMaxWriteId() const;

        std::optional<uint64_t> readMemory(uint64_t address, MemoryWrite::Id maxId);

        MemoryWrite& getWrite(MemoryWrite::Id id) const;

        void writeMemory(MemoryWrite::Id id);

        bool halted() const;

        void halt();

        void interrupt(int number);

        int interrupted() const;

        bool registerReady(PhysicalRegister reg) const;

        void setReady(PhysicalRegister reg);

        void renameRegister(Register reg);

        void renameFloatRegister(FloatRegister fReg);

        const RegisterAllocationTable& getRat() const;

        void subscribeRegisterRead(PhysicalRegister reg);

        void unsubscribeRegisterRead(PhysicalRegister reg);

        MemoryWrite::Id registerPendingWrite(Memory::Immediate mem);

        MemoryWrite::Id registerPendingWrite();

        void specifyWriteAddress(MemoryWrite::Id id, uint64_t value);

        void unrollSpeculation(const RegisterAllocationTable& rat);

        void flushPipeline();

        /// Checks if trap flag is set.
        bool isTrapFlagSet() const;
        
        /// Returns true if instruction was executed by the time trap flag
        /// was set.
        bool singleStepDone() const;

        /// Tells the CPU that single step has been completed.
        void singleStepped();

        /// Following function are for debug only
        /// In execution, version with PhysicalRegister should be used
        /// NOTE: Debug only but they are used anyway with special registers...
        ///       to be sure new function was added that will ONLY be called
        ///       for debugging purposes - getRegisterDebug.
        int64_t getRegister(Register reg) const;

        void setRegisterDebug(Register reg,
                              int64_t value);

        void setFloatRegisterDebug(FloatRegister reg,
                                   double value);

        double getFloatRegister(FloatRegister reg) const;

        void setRegister(Register reg, int64_t value);

        void setFloatRegister(FloatRegister fReg, double value);

        int64_t getMemory(uint64_t address) const;

        void setMemory(uint64_t address, int64_t value);

        size_t textSize() const { return program_.instructions().size(); }

        const Instruction& getText(uint64_t address);

        void setText(uint64_t address, std::unique_ptr<Instruction> ins);

        /// Sets trap flag
        /// TODO: Consider setting this at debugger level
        void setTrapFlag();

        /// Unsets trap flag
        /// TODO: Consider setting this at debugger level
        void unsetTrapFlag();

        bool isTrapFlagSet();

        static const size_t DEBUG_REGISTERS_CNT = 5;

        uint64_t getDebugRegister(size_t i) const {
            return debug_registers_.at(i);
        }

        void setDebugRegister(size_t i, uint64_t value) {
            debug_registers_.at(i) = value;
        }
    private:
        /// If true then after every retired instruction an interrupt 1 is sent.
        /// TODO: This should really be a part of flags register. For now however,
        /// it is a separate entity because the flags are often set with '=', which
        /// resets the value of trap flag. They are also set as part of entity, which
        /// makes it harder because it is done somewhere in the execution pipeline
        /// and is non-transparent.
        bool trapFlag_{false};

        // Branch processing
        void checkBranchPrediction(const ReservationStation::Entry& entry, uint64_t destination);

        void registerBranchNotTaken(uint64_t sourcePc);

        void registerBranchTaken(uint64_t sourcePc, uint64_t destination);

        PhysicalRegister nextFreeRegister() const;

        /// Checks if any of the writes were done to location watched by debug
        /// registers and if so then sets an interrupt.
        void checkWrite(uint64_t address);

        // Harvard architecture
        Program program_;

        uint64_t speculativeProgramCounter_{0};

        struct InstructionEntry {
            const Instruction* instruction;
            std::size_t pc;
            std::size_t loggingId;
        };

        InstructionEntry fetchInstruction();

        std::optional<InstructionEntry> instructionFetch_;

        std::optional<InstructionEntry> instructionDecode_;

        ReservationStation reservationStation_; // ReservationStations

        std::unique_ptr<BranchPredictor> branchPredictor_;


        std::size_t registerCnt_;
        std::size_t floatRegisterCnt_;
        std::size_t physicalRegisterCnt_;

        struct RegisterValue {
            int64_t value{0};
            bool ready{false};
            std::size_t subscribedReads{0};
        };

        // Values of registers, indexed by PhysicalRegister
        std::vector<RegisterValue> registers_;

        // Debug registers
        // first four bits indicate whether i-th debug reg is active.
        // 8-12 bits indicate which debug reg caused break.
        static const size_t DEBUG_CONTROL_REG_IDX = DEBUG_REGISTERS_CNT - 1;
        std::array<uint64_t, DEBUG_REGISTERS_CNT> debug_registers_;

        // Register allocation table
        RegisterAllocationTable rat_;

        RAM ram_;

        MemoryWritesManager writesManager_;

        // list of predicted jump destinations
        std::list<uint64_t> predictions_;

        std::function<void(Cpu&)> breakHandler_;

        bool halted_{false};

        bool single_stepped_{false};

        // Zero means that the program is not interrupted, any other number
        // means that interrupt has occured.
        int interrupted_{0};
    };
}
