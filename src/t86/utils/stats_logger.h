#pragma once

#include <vector>
#include <set>
#include <map>
#include <optional>
#include <unordered_map>

#include "../cpu/register.h"

namespace tiny::t86 {
    // Forward declare instruction
    class Instruction;

    class StatsLogger {
    public:
        static StatsLogger& instance();

        // Resets all the stats, should be called before every new run
        void reset();

        void newTick();

        std::size_t registerNewInstruction(std::size_t pc, const Instruction* instruction);

        void logInstructionFetch(std::size_t id);

        void logInstructionDecode(std::size_t id);

        void logNoAluAvailable(std::size_t id);

        // Waiting for register value to be available
        void logStallFetch(std::size_t id);

        void logStallRegisterFetch(std::size_t id, Register reg);

        void logStallFloatRegisterFetch(std::size_t id, FloatRegister fReg);

        // Waiting for RAM read
        void logStallRAMRead(std::size_t id, std::size_t address);

        // Waiting for previous instructions to be retired
        void logStallRetirement(std::size_t id);

        void logOperandFetching(std::size_t id);

        void logExecuting(std::size_t id);

        void logRetirement(std::size_t id);

        void logClearSpeculation(std::size_t id);

        std::size_t tickCount() const;

        void processBasicStats(std::ostream& os);

        void processDetailedStats(std::ostream& os);

        struct TickStats {
            std::optional<std::size_t> instructionFetchPc;
            std::optional<std::size_t> instructionDecodePc;
            std::vector<std::size_t> activeRSEntries;

            std::vector<std::size_t> operandFetchingRSEntries;
            std::vector<std::size_t> operandFetchingStallRSEntries;
            std::map<std::size_t, std::set<std::size_t>> stallRAMReadRSEntries;
            std::map<std::size_t, std::set<Register>> stallRegisterFetchRSEntries;
            std::map<std::size_t, std::set<FloatRegister>> stallFloatRegisterFetchRSEntries;

            std::vector<std::size_t> stallNoAluRSEntries;

            std::vector<std::size_t> executingRSEntries;

            std::vector<std::size_t> stallRetirementRSEntries;
            std::vector<std::size_t> retiredRSEntries;
        };

    protected:
        TickStats& currentTick();

        struct InstructionLifeTime {
            std::size_t fetch{0};
            std::size_t decode{0};
            std::size_t preparing{0};
            std::size_t fetchingStalls{0};
            std::map<Register, std::size_t> waitingForRegisterFetch;
            std::map<FloatRegister, std::size_t> waitingForFloatRegisterFetch;
            std::map<std::size_t, std::size_t> waitingForMemoryRead;
            std::size_t waitingForAlu{0};
            std::size_t executing{0};
            std::size_t waitingForRetirement{0};
            std::size_t retirement{0}; // Every retirement will take only one cycle, so this is excess for now, but maybe in future?

            std::size_t totalTime() const {
                return fetch + decode + preparing + waitingForAlu + executing + waitingForRetirement + retirement;
            }

            InstructionLifeTime& operator += (const InstructionLifeTime& other) {
                fetch += other.fetch;
                decode += other.decode;
                preparing += other.preparing;
                fetchingStalls += other.fetchingStalls;
                for (const auto& [reg, count] : other.waitingForRegisterFetch) {
                    waitingForRegisterFetch[reg] += count;
                }
                for (const auto& [reg, count] : other.waitingForFloatRegisterFetch) {
                    waitingForFloatRegisterFetch[reg] += count;
                }
                for (const auto& [address, count] : other.waitingForMemoryRead) {
                    waitingForMemoryRead[address] += count;
                }
                waitingForAlu += other.waitingForAlu;
                executing += other.executing;
                waitingForRetirement += other.waitingForRetirement;
                retirement += other.retirement;
                return *this;
            }
        };

        InstructionLifeTime getInstructionLifeTime(std::size_t id);

        static void processAverageLifetime(std::ostream& os, const InstructionLifeTime& lt, std::size_t totalCount);

        StatsLogger() = default;

        std::vector<TickStats> ticks_;

        // Each instruction gets its id
        std::size_t id_{0};

        // Some ids might be missing, as wrongly speculated ones will be removed
        std::unordered_map<std::size_t, std::pair<std::size_t, const Instruction*>> instructions_;
    };
}
