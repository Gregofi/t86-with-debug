#pragma once

#include "alu.h"
#include "../cpu/register.h"
#include "../cpu/register_allocation_table.h"
#include "../cpu/memory_writes_manager/memory_write.h"
#include "../utils/stats_logger.h"

#include <list>
#include <vector>
#include <optional>
#include <condition_variable>

namespace tiny::t86 {
    // Forward declaration
    class Cpu;

    class Instruction;

    class Operand;

    class ReservationStation {
    public:
        ReservationStation(Cpu& cpu, std::size_t aluCnt, std::size_t maxEntriesCnt);


        // We process executing and possibly finished instructions first
        // If they are finished, we can free the alu and forward result.
        // This helps to minimize number of iterations and follow
        // the order of execution as close to the real program as possible

        void executeAndRetire();

        void fetchAndStartExecution();

        bool hasFreeEntry() const;

        void add(const Instruction*, std::size_t nextPc, std::size_t loggingId);

        void clear();

        class Entry;

    private:
        std::list<Entry> entries_;

        const std::size_t maxEntries_;

        Cpu& cpu_;

        std::size_t freeAlus_;
    };

    class ReservationStation::Entry {
    public:
        Entry(const Instruction* instruction,
              Cpu& cpu,
              RegisterAllocationTable readRat,
              RegisterAllocationTable writeRat,
              std::vector<MemoryWrite::Id> memWriteIds,
              MemoryWrite::Id maxWriteId,
              std::size_t loggingId);

        enum class State {
            preparing, ready, executing, retiring
        };

        State state() const;

        std::vector<Operand>& operands() {
            return operands_;
        }

        const std::vector<Operand>& operands() const {
            return operands_;
        }

        const std::vector<MemoryWrite::Id>& memoryWriteIds() const {
            return memWriteIds_;
        }

        void retire();

        void checkReady();

        void startExecution();

        bool executionTick();

        const Instruction* instruction() const;

        const RegisterAllocationTable& rat() const;

        void unrollSpeculation();

        Cpu& cpu() const;

        bool registerAvailable(Register reg) const;

        bool floatRegisterAvailable(FloatRegister fReg) const;

        int64_t getRegister(Register reg) const;

        double getFloatRegister(FloatRegister fReg) const;

        void setRegister(Register reg, int64_t val);

        void setFloatRegister(FloatRegister fReg, double val);

        uint64_t getUpdatedProgramCounter() const;

        std::optional<int64_t> readMemory(uint64_t address);

        void specifyWriteAddress(MemoryWrite::Id id, std::size_t address);

        void setWriteValue(MemoryWrite::Id id, uint64_t value);

        void writeMemory(MemoryWrite::Id);

        void processJump(bool taken);

        void setProgramCounter(uint64_t address);

        void setFlags(Alu::Flags flags);

        void setStackPointer(uint64_t address);

        // TODO maybe remove, not really used
        void setStackBasePointer(uint64_t address);

        // Logging
        void logClearSpeculation() const;

        void logPreparing() const;

        void logStallFetch() const;

        void logStallRegisterFetch(Register reg) const;

        void logStallFloatRegisterFetch(FloatRegister fReg) const;

        void logStallRAMRead(uint64_t address) const;

        void logStallALU() const;

        void logExecuting() const;

        void logStallRetirement() const;

        void logRetirement() const;

    private:
        bool allOperandsFetched() const;

        const Instruction* instruction_;

        std::vector<Operand> operands_;

        RegisterAllocationTable readRat_;

        RegisterAllocationTable writeRat_;

        std::vector<MemoryWrite::Id> memWriteIds_;

        MemoryWrite::Id maxWriteId_;

        Cpu& cpu_;

        State state_ = State::preparing;

        size_t remainingExecutionTime_;

        std::size_t loggingId_;

        std::exception_ptr memoryAccessException_;
    };
}
