#include "stats_logger.h"

#include "../instruction.h"

#include <iostream>
#include <cassert>
#include <unordered_map>

namespace tiny::t86 {
    void StatsLogger::logInstructionFetch(std::size_t id) {
        currentTick().instructionFetchPc = id;
    }

    void StatsLogger::logInstructionDecode(std::size_t id) {
        currentTick().instructionDecodePc = id;
    }

    void StatsLogger::logStallRetirement(std::size_t id) {
        currentTick().stallRetirementRSEntries.push_back(id);
    }

    StatsLogger& StatsLogger::instance() {
        static StatsLogger instance;
        return instance;
    }

    void StatsLogger::logNoAluAvailable(std::size_t id) {
        currentTick().stallNoAluRSEntries.push_back(id);
    }

    void StatsLogger::newTick() {
        ticks_.emplace_back();
    }

    std::size_t StatsLogger::tickCount() const {
        return ticks_.size();
    }

    void StatsLogger::processBasicStats(std::ostream& os) {
        std::size_t totalTicks = ticks_.size();
        std::size_t totalInstructions = instructions_.size();
        std::unordered_map<std::size_t, InstructionLifeTime> lifetimes;
        std::map<Instruction::Signature, std::pair<InstructionLifeTime, std::size_t>> lifetimesBySignature;
        InstructionLifeTime accumulativeInstructionLifeTime;
        for (const auto& [id, ins] : instructions_) {
            InstructionLifeTime lt = getInstructionLifeTime(id);
            accumulativeInstructionLifeTime += lt;
            lifetimes.emplace(id, lt);
            auto& signatureEntry = lifetimesBySignature[instructions_.at(id).second->getSignature()];
            signatureEntry.first += lt;
            signatureEntry.second += 1;
        }
        os << "------------------------------------------\n";
        os << "Total ticks: " << totalTicks << std::endl;
        os << "Total instructions executed: " << instructions_.size() << std::endl;
        double throughput = static_cast<double>(instructions_.size()) / totalTicks;
        os << "Throughput: " << throughput << " instructions per tick\n";
        os << "Average instruction latency: " << 1 / throughput << " ticks\n";
        os << "Global averages:\n";
        processAverageLifetime(os, accumulativeInstructionLifeTime, totalInstructions);
        std::cerr << std::flush;
    }

    void StatsLogger::processDetailedStats(std::ostream& os) {
        std::size_t totalTicks = ticks_.size();
        std::size_t totalInstructions = instructions_.size();
        std::unordered_map<std::size_t, InstructionLifeTime> lifetimes;
        std::map<Instruction::Signature, std::pair<InstructionLifeTime, std::size_t>> lifetimesBySignature;
        InstructionLifeTime accumulativeInstructionLifeTime;
        for (const auto& [id, ins] : instructions_) {
            InstructionLifeTime lt = getInstructionLifeTime(id);
            accumulativeInstructionLifeTime += lt;
            lifetimes.emplace(id, lt);
            auto& signatureEntry = lifetimesBySignature[instructions_.at(id).second->getSignature()];
            signatureEntry.first += lt;
            signatureEntry.second += 1;
        }
        os << "------------------------------------------\n";
        for (const auto& [signature, entry] : lifetimesBySignature) {
            const auto& [lt, count] = entry;
            os << "Averages for " << signature.toString() << ":\n";
            processAverageLifetime(os, lt, count);
        }
    }

    void StatsLogger::reset() {
        ticks_.clear();
        instructions_.clear();
        id_ = 0;
    }

    StatsLogger::TickStats& StatsLogger::currentTick() {
        return ticks_.back();
    }

    void StatsLogger::logOperandFetching(std::size_t id) {
        currentTick().operandFetchingRSEntries.push_back(id);
    }

    void StatsLogger::logStallFetch(std::size_t id) {
        currentTick().operandFetchingStallRSEntries.push_back(id);
    }

    void StatsLogger::logStallRegisterFetch(std::size_t id, Register reg) {
        currentTick().stallRegisterFetchRSEntries[id].insert(reg);
    }

    void StatsLogger::logStallFloatRegisterFetch(std::size_t id, FloatRegister fReg) {
        currentTick().stallFloatRegisterFetchRSEntries[id].insert(fReg);
    }

    void StatsLogger::logStallRAMRead(std::size_t id, std::size_t address) {
        currentTick().stallRAMReadRSEntries[id].insert(address);
    }

    void StatsLogger::logExecuting(std::size_t id) {
        currentTick().executingRSEntries.push_back(id);
    }

    void StatsLogger::logRetirement(std::size_t id) {
        currentTick().retiredRSEntries.push_back(id);
    }

    void StatsLogger::processAverageLifetime(std::ostream& os, const StatsLogger::InstructionLifeTime& lt, std::size_t totalCount) {
        os << "  Times executed: " << totalCount << '\n'
           << "  Average instruction lifetime: " << static_cast<double>(lt.totalTime()) / totalCount << " ticks\n"
           << "    Average fetch stalls: " << static_cast<double>(lt.fetch - 1) / totalCount << " ticks\n"
           << "    Average decode stalls: " << static_cast<double>(lt.decode - 1) / totalCount << " ticks\n"
           << "    Average operand fetching stalls: " << static_cast<double>(lt.fetchingStalls) / totalCount << " ticks\n";

        std::size_t totalWaitingForRegisters = 0;
        for (const auto& [reg, count] : lt.waitingForRegisterFetch) {
            totalWaitingForRegisters += count;
        }
        if (totalWaitingForRegisters != 0) {
            os << "      Average register fetch stalls: " << static_cast<double>(totalWaitingForRegisters) / totalCount << " ticks\n";
        }

        std::size_t totalWaitingForFloatRegisters = 0;
        for (const auto& [reg, count] : lt.waitingForFloatRegisterFetch) {
            totalWaitingForFloatRegisters += count;
        }
        if (totalWaitingForFloatRegisters != 0) {
            os << "      Average float register fetch stalls: " << static_cast<double>(totalWaitingForFloatRegisters) / totalCount << " ticks\n";
        }

        std::size_t totalWaitingForMemory = 0;
        for (const auto& [address, count] : lt.waitingForMemoryRead) {
            totalWaitingForMemory += count;
        }
        if (totalWaitingForMemory != 0) {
            os << "      Average memory read stalls: " << static_cast<double>(totalWaitingForMemory) / totalCount << " ticks\n";
        }

        os << "    Average waiting for ALU: " << static_cast<double>(lt.waitingForAlu) / totalCount << " ticks\n"
           << "    Average executing: " << static_cast<double>(lt.executing) / totalCount << " ticks\n"
           << "    Average waiting for retirement: " << static_cast<double>(lt.waitingForRetirement) / totalCount << " ticks\n"
           << "    Average retirement: " << static_cast<double>(lt.retirement) / totalCount << " ticks\n";
    }

    std::size_t StatsLogger::registerNewInstruction(std::size_t pc, const Instruction* instruction) {
        instructions_.emplace(id_, std::make_pair(pc, instruction));
        return id_++;
    }

    void StatsLogger::logClearSpeculation(std::size_t id) {
        instructions_.erase(id);
    }

    StatsLogger::InstructionLifeTime StatsLogger::getInstructionLifeTime(std::size_t id) {
        InstructionLifeTime lifeTime;
        auto it = ticks_.cbegin();
        // Skip to where the instruction appears for the first time
        while(it != ticks_.end() && it->instructionFetchPc != id) {
            ++it;
        }
        assert(it != ticks_.end());
        while(it != ticks_.end() && it->instructionFetchPc == id) {
            ++lifeTime.fetch;
            ++it;
        }
        while(it != ticks_.end() && it->instructionDecodePc == id) {
            ++lifeTime.decode;
            ++it;
        }
        while(it != ticks_.end() &&
              // Please, C++20 be here soon, so I can just replace this with contains
              std::find(it->operandFetchingRSEntries.begin(), it->operandFetchingRSEntries.end(), id) != it->operandFetchingRSEntries.end()) {
            ++lifeTime.preparing;
            if (std::find(it->operandFetchingStallRSEntries.begin(), it->operandFetchingStallRSEntries.end(), id) != it->operandFetchingStallRSEntries.end()) {
                ++lifeTime.fetchingStalls;
            }
            if (auto rsIt = it->stallRegisterFetchRSEntries.find(id); rsIt != it->stallRegisterFetchRSEntries.end()) {
                for (Register reg : rsIt->second) {
                    ++lifeTime.waitingForRegisterFetch[reg];
                }
            }
            if (auto frsIt = it->stallFloatRegisterFetchRSEntries.find(id); frsIt != it->stallFloatRegisterFetchRSEntries.end()) {
                for (FloatRegister fReg : frsIt->second) {
                    ++lifeTime.waitingForFloatRegisterFetch[fReg];
                }
            }
            if (auto memIt = it->stallRAMReadRSEntries.find(id); memIt != it->stallRAMReadRSEntries.end()) {
                for (std::size_t address : memIt->second) {
                    ++lifeTime.waitingForMemoryRead[address];
                }
            }
            ++it;
        }
        while(it != ticks_.end() &&
              std::find(it->stallNoAluRSEntries.begin(), it->stallNoAluRSEntries.end(), id) != it->stallNoAluRSEntries.end()) {
            ++lifeTime.waitingForAlu;
            ++it;
        }
        while(it != ticks_.end() && std::find(it->executingRSEntries.begin(), it->executingRSEntries.end(), id) != it->executingRSEntries.end()) {
            ++lifeTime.executing;
            ++it;
        }
        while(it != ticks_.end() && std::find(it->stallRetirementRSEntries.begin(), it->stallRetirementRSEntries.end(), id) != it->stallRetirementRSEntries.end()) {
            ++lifeTime.waitingForRetirement;
            ++it;
        }
        assert(it != ticks_.end());
        assert(std::find(it->retiredRSEntries.begin(), it->retiredRSEntries.end(), id) != it->retiredRSEntries.end());
        ++lifeTime.retirement;
        return lifeTime;
    }
}
