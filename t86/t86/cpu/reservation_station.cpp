#include "reservation_station.h"
#include "../cpu.h"
#include "../utils/stats_logger.h"

#include <cassert>
#include <stdexcept>

namespace tiny::t86 {
    void ReservationStation::executeAndRetire() {
        // First check finished ones by progressing execution
        for (auto& entry : entries_) {
            if (entry.state() == Entry::State::executing) {
                if (entry.executionTick()) {
                    // finished
                    if (entry.instruction()->needsAlu()) {
                        ++freeAlus_;
                    }
                }
            }
        }

        // List of entries are in order of execution
        // We start from the start and retire until first unfinished instruction is met
        // It is better to do this in this while loop, because retiring conditional jump instruction
        // might lead to erasure of all other instructions in reservation station (invalidating all iterators)
        // Instructions that just ended execution can retire also in this tick
        // but one tick was also "taken" by preparing state
        while (!entries_.empty()) {
            if (entries_.front().state() == Entry::State::retiring) {
                Entry entry = std::move(entries_.front());
                entries_.pop_front();
                entry.logRetirement();
                entry.retire();
            }
            else {
                break;
            }
        }
    }

    void ReservationStation::fetchAndStartExecution() {
        // Loop through the rest and update them
        for (auto& entry : entries_) {
            switch (entry.state()) {
                case Entry::State::preparing: {
                    entry.logPreparing();
                    bool fetchStall{false};
                    for (Operand& operand : entry.operands()) {
                        // Check if we need to fetch and fetch as much as we can right now
                        while(!operand.isFetched()) {
                            // Get the requirement
                            Requirement requirement = operand.requirement();
                            if (requirement.isRegisterRead()) {
                                Register reg = requirement.getRegisterRead();
                                if (entry.registerAvailable(reg)) {
                                    operand.supply(entry.getRegister(reg));
                                } else {
                                    fetchStall = true;
                                    entry.logStallRegisterFetch(reg);
                                    // This following is very hacky, it's here only for better logging
                                    // This assumes that there are max 2 register in operands
                                    // We don't really care for memory, the mem will not start fetching until we know the address, that can be made of 2 registers    
                                    // COPY the operand, not to mess up the real operand
                                    Operand op = operand;
                                    // supply dummy value
                                    op.supply((int64_t)0);
                                    // Check it still needs another register
                                    if (!op.isFetched()) {
                                        Requirement req = op.requirement();
                                        if (req.isRegisterRead()) {
                                            // Another register
                                            Register r = req.getRegisterRead();
                                            // check if available
                                            if (!entry.registerAvailable(r)) {
                                                // We log this one as well
                                                entry.logStallRegisterFetch(r);
                                            }
                                        }
                                    }
                                    break;
                                }
                            } else if (requirement.isFloatRegisterRead()) {
                                FloatRegister fReg = requirement.getFloatRegisterRead();
                                if (entry.floatRegisterAvailable(fReg)) {
                                    operand.supply(entry.getFloatRegister(fReg));
                                } else {
                                    fetchStall = true;
                                    entry.logStallFloatRegisterFetch(fReg);
                                    break;
                                }
                            } else if (requirement.isMemoryRead()) {
                                uint64_t address = requirement.getMemoryRead();
                                auto optMemory = entry.readMemory(address);
                                if (optMemory.has_value()) {
                                    operand.supply(optMemory.value());
                                } else {
                                    fetchStall = true;
                                    entry.logStallRAMRead(address);
                                    break;
                                }
                            } else {
                                assert(false && "Unhandled requirement type");
                            }
                        }
                    }
                    if (fetchStall) {
                        entry.logStallFetch();
                    }
                    // Check if all operands fetched
                    entry.checkReady();
                    break;
                }
                case Entry::State::ready:
                    // Check for ALU
                    if (entry.instruction()->needsAlu()) {
                        // No ALU is free
                        if (!freeAlus_) {
                            entry.logStallALU();
                            break;
                        }
                        --freeAlus_;
                    }
                    // Start execution
                    // Again, this will result into one tick spent in "ready" state
                    entry.startExecution();
                    // This transition can be interpreted as already executing
                    [[fallthrough]];
                case Entry::State::executing:
                    // Nothing to do here, we already processed it before
                    // Just log here
                    entry.logExecuting();
                    break;
                case Entry::State::retiring:
                    // If there are some entries at this point, it means that there are some instructions before, that "block" this instruction
                    // from retirement
                    entry.logStallRetirement();
                    break;
            }
        }
    }

    bool ReservationStation::hasFreeEntry() const {
        return entries_.size() < maxEntries_;
    }

    ReservationStation::ReservationStation(Cpu& cpu, std::size_t aluCnt, std::size_t maxEntriesCnt)
            : maxEntries_(maxEntriesCnt), cpu_(cpu), freeAlus_(aluCnt) {}

    void ReservationStation::add(const Instruction* instruction, std::size_t nextPc, std::size_t loggingId) {
        assert(entries_.size() < maxEntries_ && "Can't add another entry, max capacity was reached");
        cpu_.renameRegister(Register::ProgramCounter());
        cpu_.setRegister(Register::ProgramCounter(), nextPc);
        RegisterAllocationTable readRat = cpu_.getRat();
        std::vector<MemoryWrite::Id> memWriteIds;
        for (const auto& product : instruction->produces()) {
            if (product.isRegister()) {
                Register reg = product.getRegister();
                // We always rename program counter
                if (reg != Register::ProgramCounter()) {
                    cpu_.renameRegister(reg);
                }
            } else if (product.isFloatRegister()) {
                FloatRegister fReg = product.getFloatRegister();
                cpu_.renameFloatRegister(fReg);
            } else if (product.isMemoryImmediate()) {
                memWriteIds.push_back(cpu_.registerPendingWrite(product.getMemoryImmediate()));
            } else if (product.isMemoryRegister()) {
                memWriteIds.push_back(cpu_.registerPendingWrite());
            } else {
                assert(false && "Missing product type");
            }
        }
        RegisterAllocationTable writeRat = cpu_.getRat();
        auto& entry = entries_.emplace_back(instruction, cpu_,
                              std::move(readRat), std::move(writeRat),
                              std::move(memWriteIds), cpu_.currentMaxWriteId(),
                              loggingId);

        // Log as preparing status
        entry.logPreparing();
        // Check if ready (for some instructions that do not have any operands)
        entry.checkReady();
    }

    void ReservationStation::clear() {
        for (const auto& entry : entries_) {
            if (entry.state() == Entry::State::executing && entry.instruction()->needsAlu()) {
                ++freeAlus_;
            }
            entry.logClearSpeculation();
        }
        entries_.clear();
    }

    bool ReservationStation::Entry::registerAvailable(Register reg) const {
        return cpu_.registerReady(readRat_.translate(reg));
    }

    bool ReservationStation::Entry::floatRegisterAvailable(FloatRegister fReg) const {
        return cpu_.registerReady(readRat_.translate(fReg));
    }

    int64_t ReservationStation::Entry::getRegister(Register reg) const {
        assert(registerAvailable(reg));
        return cpu_.getRegister(readRat_.translate(reg));
    }

    double ReservationStation::Entry::getFloatRegister(FloatRegister fReg) const {
        assert(floatRegisterAvailable(fReg));
        return cpu_.getFloatRegister(readRat_.translate(fReg));
    }

    void ReservationStation::Entry::setRegister(Register reg, int64_t val) {
        assert(reg == Register::ProgramCounter() || !cpu_.registerReady(writeRat_.translate(reg)));
        cpu_.setRegister(writeRat_.translate(reg), val);
    }

    void ReservationStation::Entry::setFloatRegister(FloatRegister fReg, double val) {
        cpu_.setRegister(writeRat_.translate(fReg), val);
    }

    uint64_t ReservationStation::Entry::getUpdatedProgramCounter() const {
        return cpu_.getRegister(writeRat_.translate(Register::ProgramCounter()));
    }

    void ReservationStation::Entry::processJump(bool taken) {
        cpu_.jump(*this, taken);
    }

    void ReservationStation::Entry::setProgramCounter(uint64_t address) {
        setRegister(Register::ProgramCounter(), address);
    }

    void ReservationStation::Entry::setFlags(Alu::Flags flags) {
        setRegister(Register::Flags(), flags);
    }

    void ReservationStation::Entry::setStackPointer(uint64_t address) {
        setRegister(Register::StackPointer(), address);
    }

    void ReservationStation::Entry::setStackBasePointer(uint64_t address) {
        setRegister(Register::StackBasePointer(), address);
    }

    ReservationStation::Entry::Entry(const Instruction* instruction, Cpu& cpu,
                                     RegisterAllocationTable readRat, RegisterAllocationTable writeRat,
                                     std::vector<MemoryWrite::Id> memWriteIds,
                                     MemoryWrite::Id maxWriteId,
                                     std::size_t loggingId)
            : instruction_(instruction),
              operands_(instruction->operands()),
              readRat_(std::move(readRat)),
              writeRat_(std::move(writeRat)),
              memWriteIds_(std::move(memWriteIds)),
              maxWriteId_(maxWriteId),
              cpu_(cpu),
              loggingId_(loggingId) {
        remainingExecutionTime_ = Cpu::Config::instance().getExecutionLength(instruction);
    }

    bool ReservationStation::Entry::allOperandsFetched() const {
        return std::all_of(operands_.begin(), operands_.end(),
                           [](const Operand& operand) { return operand.isFetched(); });
    }

    bool ReservationStation::Entry::executionTick() {
        assert(state_ == State::executing);

        if (remainingExecutionTime_ != 0) {
            --remainingExecutionTime_;
        }
        // This is done "two-steps" because some instructions might have zero execution tickCount required
        if (remainingExecutionTime_ == 0) {
            instruction_->execute(*this);
            state_ = State::retiring;
            return true;
        }
        return false;
    }

    ReservationStation::Entry::State ReservationStation::Entry::state() const {
        return state_;
    }

    const Instruction* ReservationStation::Entry::instruction() const {
        return instruction_;
    }

    void ReservationStation::Entry::startExecution() {
        assert(state_ == State::ready && "Starting execution on instruction that is not in ready state");
        state_ = State::executing;
    }

    void ReservationStation::Entry::checkReady() {
        assert(state_ == State::preparing && "Instruction should be in preparing state while checking if ready");
        if (allOperandsFetched()) {
            state_ = State::ready;
        }
    }

    void ReservationStation::Entry::retire() {
        instruction_->retire(*this);
    }

    Cpu& ReservationStation::Entry::cpu() const {
        return cpu_;
    }

    void ReservationStation::Entry::writeMemory(MemoryWrite::Id id) {
        cpu_.writeMemory(id);
    }

    const RegisterAllocationTable& ReservationStation::Entry::rat() const {
        // Returning the write rat, because we moved some registers
        // Used mostly when fixing branch prediction miss, so we care mainly for the PC
        return writeRat_;
    }

    void ReservationStation::Entry::unrollSpeculation() {
        cpu_.unrollSpeculation(writeRat_);
    }

    std::optional<int64_t> ReservationStation::Entry::readMemory(uint64_t address) {
        return cpu_.readMemory(address, maxWriteId_);
    }

    void ReservationStation::Entry::specifyWriteAddress(MemoryWrite::Id id, std::size_t address) {
        cpu_.specifyWriteAddress(id, address);
    }

    void ReservationStation::Entry::setWriteValue(MemoryWrite::Id id, uint64_t value) {
        cpu_.getWrite(id).setValue(value);
    }

    void ReservationStation::Entry::logClearSpeculation() const {
        StatsLogger::instance().logClearSpeculation(loggingId_);
    }

    void ReservationStation::Entry::logExecuting() const {
        StatsLogger::instance().logExecuting(loggingId_);
    }

    void ReservationStation::Entry::logPreparing() const {
        StatsLogger::instance().logOperandFetching(loggingId_);
    }

    void ReservationStation::Entry::logStallFetch() const {
        StatsLogger::instance().logStallFetch(loggingId_);
    }

    void ReservationStation::Entry::logStallRegisterFetch(Register reg) const {
        StatsLogger::instance().logStallRegisterFetch(loggingId_, reg);
    }

    void ReservationStation::Entry::logStallFloatRegisterFetch(FloatRegister fReg) const {
        StatsLogger::instance().logStallFloatRegisterFetch(loggingId_, fReg);
    }

    void ReservationStation::Entry::logStallRAMRead(uint64_t address) const {
        StatsLogger::instance().logStallRAMRead(loggingId_, address);
    }

    void ReservationStation::Entry::logStallRetirement() const {
        StatsLogger::instance().logStallRetirement(loggingId_);
    }

    void ReservationStation::Entry::logRetirement() const {
        StatsLogger::instance().logRetirement(loggingId_);
    }

    void ReservationStation::Entry::logStallALU() const {
        StatsLogger::instance().logNoAluAvailable(loggingId_);
    }
}