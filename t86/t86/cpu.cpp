#include <functional>
#include <utility>
#include <cassert>

#include "cpu.h"
#include "utils/stats_logger.h"
#include "cpu/branch_predictors/naive_branch_predictor.h"
#include "../common/config.h"

namespace tiny::t86 {
    void Cpu::tick() {
        StatsLogger::instance().newTick();

        ram_.tick();

        writesManager_.removeFinished(ram_);

        reservationStation_.executeAndRetire();

        if (halted()) {
            return;
        }

        reservationStation_.fetchAndStartExecution();

        if (instructionDecode_) {
            if (reservationStation_.hasFreeEntry()) {
                reservationStation_.add(instructionDecode_->instruction, instructionDecode_->pc, instructionDecode_->loggingId);
                instructionDecode_ = std::nullopt;
            }
        }

        if (!instructionDecode_) {
            // this will set instructionFetch to be nullopt
            std::swap(instructionDecode_, instructionFetch_);
        }

        if (!instructionFetch_) {
            instructionFetch_ = fetchInstruction();
        }

        if (instructionFetch_) {
            StatsLogger::instance().logInstructionFetch(instructionFetch_->loggingId);
        }
        if (instructionDecode_) {
            StatsLogger::instance().logInstructionDecode(instructionDecode_->loggingId);
        }
    }

    Cpu::InstructionEntry Cpu::fetchInstruction() {
        std::size_t oldPc = speculativeProgramCounter_;
        const auto* instruction = program_.at(speculativeProgramCounter_);
        if (auto jumpInstruction = dynamic_cast<const JumpInstruction*>(instruction); jumpInstruction) {
            speculativeProgramCounter_ = branchPredictor_->nextGuess(speculativeProgramCounter_, *jumpInstruction);
            predictions_.push_back(speculativeProgramCounter_);
        }
        else {
            ++speculativeProgramCounter_;
        }
        return {instruction, oldPc + 1, StatsLogger::instance().registerNewInstruction(oldPc, instruction)};
    }

    int64_t Cpu::getRegister(Register reg) const {
        return getRegister(rat_.translate(reg));
    }

    double Cpu::getFloatRegister(FloatRegister fReg) const {
        return getFloatRegister(rat_.translate(fReg));
    }

    int64_t Cpu::getRegister(PhysicalRegister reg) const {
        return registers_.at(reg.index()).value;
    }

    double Cpu::getFloatRegister(PhysicalRegister reg) const {
        int64_t val = registers_.at(reg.index()).value;
        return *reinterpret_cast<double*>(&val);
    }

    Cpu::Cpu(std::size_t registerCount, std::size_t floatRegisterCount, std::size_t aluCnt)
            : Cpu(registerCount, floatRegisterCount, aluCnt, aluCnt * 2, Config::instance().ramSize(), Config::instance().ramGatesCount()) {}

    Cpu::Cpu(std::size_t registerCount, std::size_t floatRegisterCount, std::size_t aluCnt, std::size_t reservationStationEntriesCount,
        std::size_t ramSize, std::size_t ramGatesCnt)
            : reservationStation_(*this, aluCnt, reservationStationEntriesCount),
              branchPredictor_{std::make_unique<NaiveBranchPredictor>()},
              registerCnt_(registerCount),
              floatRegisterCnt_(floatRegisterCount),
              physicalRegisterCnt_(specialRegistersCnt + registerCount + floatRegisterCount + reservationStationEntriesCount * possibleRenamedRegisterCnt),
              registers_(physicalRegisterCnt_),
              rat_(*this, registerCount, floatRegisterCount),
              ram_(ramSize, ramGatesCnt)
    {
        // To be sure, theoretically not needed
        for (std::size_t i = 0; i < registerCount; ++i) {
            setRegister(Register{i}, 0);
        }
        setRegister(Register::ProgramCounter(), 0);
        setRegister(Register::Flags(), 0);
        setRegister(Register::StackPointer(), ram_.size());
        setRegister(Register::StackBasePointer(), ram_.size());

    }

    void Cpu::setRegister(Register reg, int64_t value) {
        setRegister(rat_.translate(reg), value);
    }

    void Cpu::setFloatRegister(FloatRegister fReg, double value) {
        setRegister(rat_.translate(fReg), value);
    }

    void Cpu::setRegister(PhysicalRegister reg, int64_t value) {
        registers_.at(reg.index()).value = value;
        registers_.at(reg.index()).ready = true;
    }

    void Cpu::setRegister(PhysicalRegister reg, double value) {
        registers_.at(reg.index()).value = *reinterpret_cast<int64_t*>(&value); // Store the double as int64_t
        registers_.at(reg.index()).ready = true;
    }

    void Cpu::start(Program&& program) {
        program_ = std::move(program);
        const auto& data = program_.data();
        for (std::size_t i = 0; i < data.size(); ++i) {
            setMemory(i, data[i]);
        }
    }

    bool Cpu::halted() const {
        return halted_;
    }

    void Cpu::halt() {
        halted_ = true;
    }

    std::optional<uint64_t> Cpu::readMemory(uint64_t address, MemoryWrite::Id maxId) {
        if (writesManager_.hasUnspecifiedWrites(maxId)) {
            return std::nullopt;
        }
        auto optWrite = writesManager_.previousWrite(address, maxId);
        if (optWrite) {
            // There is a previous write
            if (optWrite->hasValue()) {
                // We already know the value
                return optWrite->value();
            }
            // We don't know the value yet
            return std::nullopt;
        }
        // We need to read it from mem
        return ram_.read(address);
    }

    void Cpu::writeMemory(MemoryWrite::Id id) {
        writesManager_.startWriting(id, ram_);
    }

    uint64_t Cpu::getMemory(uint64_t address) const {
        return ram_.get(address);
    }

    void Cpu::setMemory(uint64_t address, uint64_t value) {
        ram_.set(address, value);
    }

    void Cpu::jump(const ReservationStation::Entry& entry, bool taken) {
        uint64_t destination = entry.getUpdatedProgramCounter();
        if (taken) {
            registerBranchTaken(entry.getRegister(Register::ProgramCounter()), destination);
        } else {
            registerBranchNotTaken(entry.getRegister(Register::ProgramCounter()));
        }
        checkBranchPrediction(entry, destination);
    }

    void Cpu::registerBranchTaken(uint64_t sourcePc, uint64_t destination) {
        branchPredictor_->registerBranchTaken(sourcePc, destination);
    }

    void Cpu::registerBranchNotTaken(uint64_t sourcePc) {
        branchPredictor_->registerBranchNotTaken(sourcePc);
    }

    void Cpu::checkBranchPrediction(const ReservationStation::Entry& entry, uint64_t destination) {
        assert(!predictions_.empty());
        std::size_t predictedDestination = predictions_.front();
        predictions_.pop_front();
        if (predictedDestination != destination) {
            unrollSpeculation(entry.rat());
        }
    }

    bool Cpu::registerReady(PhysicalRegister reg) const {
        return registers_.at(reg.index()).ready;
    }

    void Cpu::setReady(PhysicalRegister reg) {
        assert(!registerReady(reg));
        registers_.at(reg.index()).ready = true;
    }

    void Cpu::connectBreakHandler(std::function<void(Cpu&)> handler) {
        breakHandler_ = std::move(handler);
    }

    void Cpu::doBreak() {
        if (breakHandler_) {
            breakHandler_(*this);
        }
    }

    const RegisterAllocationTable& Cpu::getRat() const {
        return rat_;
    }

    void Cpu::subscribeRegisterRead(PhysicalRegister reg) {
        ++(registers_.at(reg.index()).subscribedReads);
    }

    void Cpu::unsubscribeRegisterRead(PhysicalRegister reg) {
        assert(registers_.at(reg.index()).subscribedReads);
        --(registers_.at(reg.index()).subscribedReads);
    }

    void Cpu::renameRegister(Register reg) {
        PhysicalRegister dest = nextFreeRegister();
        rat_.rename(reg, dest);
        registers_.at(dest.index()).ready = false;
    }

    void Cpu::renameFloatRegister(FloatRegister fReg) {
        PhysicalRegister dest = nextFreeRegister();
        rat_.rename(fReg, dest);
        registers_.at(dest.index()).ready = false;
    }

    PhysicalRegister Cpu::nextFreeRegister() const {
        for (std::size_t i = 0; i < physicalRegisterCnt_; ++i) {
            if (rat_.isUnmapped(PhysicalRegister{i}) && registers_.at(i).subscribedReads == 0) {
                return i;
            }
        }
        throw std::runtime_error("No free register was found, either bug in RAT or small scale for physical registers");
    }

    void Cpu::flushPipeline() {
        // Unroll speculation
        reservationStation_.clear();
        predictions_.clear();
        if (instructionFetch_) {
            StatsLogger::instance().logClearSpeculation(instructionFetch_->loggingId);
            instructionFetch_ = std::nullopt;
        }
        if (instructionDecode_) {
            StatsLogger::instance().logClearSpeculation(instructionDecode_->loggingId);
            instructionDecode_ = std::nullopt;
        }
    }

    void Cpu::unrollSpeculation(const RegisterAllocationTable& rat) {
        flushPipeline();
        // Restore rat
        rat_ = rat;

        // Set correct PC
        speculativeProgramCounter_ = getRegister(Register::ProgramCounter());

        // Remove pending writes
        writesManager_.removePending();
    }

    Cpu::Cpu() : Cpu(Cpu::Config::instance().registerCnt(),
                     Cpu::Config::instance().aluCnt(),
                     Cpu::Config::instance().reservationStationEntriesCnt()) {}

    MemoryWrite::Id Cpu::registerPendingWrite(Memory::Immediate mem) {
        return writesManager_.registerPendingWrite(mem.index());
    }

    MemoryWrite::Id Cpu::registerPendingWrite() {
        return writesManager_.registerPendingWrite();
    }

    MemoryWrite& Cpu::getWrite(MemoryWrite::Id id) const {
        return writesManager_.getWrite(id);
    }

    MemoryWrite::Id Cpu::currentMaxWriteId() const {
        return writesManager_.currentMaxWriteId();
    }

    void Cpu::specifyWriteAddress(MemoryWrite::Id id, uint64_t value) {
        writesManager_.specifyAddress(id, value);
    }

    std::size_t Cpu::Config::registerCnt() const {
        return std::stoul(config.get(registerCountConfigString));
    }

    std::size_t Cpu::Config::floatRegisterCnt() const {
        return std::stoul(config.get(floatRegisterCountConfigString));
    }

    std::size_t Cpu::Config::aluCnt() const {
        return std::stoul(config.get(aluCountConfigString));
    }

    std::size_t Cpu::Config::reservationStationEntriesCnt() const {
        return std::stoul(config.get(reservationStationEntriesCountConfigString));
    }

    std::size_t Cpu::Config::ramSize() const {
        return std::stoul(config.get(ramSizeConfigString));
    }

    std::size_t Cpu::Config::ramGatesCount() const {
        return std::stoul(config.get(ramGatesCountConfigString));
    }

    std::size_t Cpu::Config::getExecutionLength(const Instruction* ins) const {
        static std::map<Instruction::Signature, std::size_t> lengths = {
            { { Instruction::Type::MOV, { Operand::Type::Reg, Operand::Type::Imm } }, 2 },
        };

        if (auto it = lengths.find(ins->getSignature()); it != lengths.end()) {
            return it->second;
        } else {
            // DEFAULT VALUE
            return 3;
        }
    }

    Cpu::Config::Config() {
        config.setDefaultIfMissing(Config::registerCountConfigString,
                                   std::to_string(Config::defaultRegisterCount));
        config.setDefaultIfMissing(Config::floatRegisterCountConfigString,
                                   std::to_string(Config::defaultFloatRegisterCount));
        config.setDefaultIfMissing(Config::aluCountConfigString,
                                   std::to_string(Config::defaultAluCount));
        config.setDefaultIfMissing(Config::reservationStationEntriesCountConfigString,
                                   std::to_string(Config::defaultReservationStationEntriesCount));
        config.setDefaultIfMissing(Config::ramSizeConfigString,
                                   std::to_string(Config::defaultRamSize));
        config.setDefaultIfMissing(Config::ramGatesCountConfigString,
                                   std::to_string(Config::defaultRamGatesCount));
    }
}