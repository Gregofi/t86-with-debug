#include <functional>
#include <utility>
#include <cassert>

#include "cpu.h"
#include "execution_error.h"
#include "utils/stats_logger.h"
#include "cpu/branch_predictors/naive_branch_predictor.h"
#include "common/config.h"
#include "common/logger.h"

namespace tiny::t86 {
    void Cpu::tick() {
        log_debug("main: flag register value: {:x}", getRegister(Register::Flags()));
        log_debug("Beginning of tick");
        // Clear interrupt flag
        interrupted_ = 0;

        StatsLogger::instance().newTick();

        ram_.tick();

        writesManager_.removeFinished(ram_);

        reservationStation_.executeAndRetire();

        if (halted()) {
            return;
        }

        if (singleStepDone()) {
            log_info("Breaking on single step!");
            single_stepped_ = false;
            // It's possible that some other condition occured while we're single stepping (for example write
            // onto watched memory cell). In that case do not overwrite that interrupt.
            if (interrupted_ == 0) {
                interrupted_ = 1;
            }
        }

        if (interrupted_ != 0) {
            log_info("Stop execution because of interrupt");
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

        log_debug("End of tick");
    }

    Cpu::InstructionEntry Cpu::fetchInstruction() {
        std::size_t oldPc = speculativeProgramCounter_;
        const auto* instruction = &program_.at(speculativeProgramCounter_);
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
        return utils::reinterpret_safe<double>(val);
    }

    Cpu::Cpu(size_t registerCount)
        : Cpu(registerCount, Cpu::Config::instance().floatRegisterCnt(),
              Cpu::Config::instance().ramSize()) {}

    Cpu::Cpu(size_t registerCount, size_t floatRegisterCount)
        : Cpu(registerCount, floatRegisterCount,
              Cpu::Config::instance().ramSize()) {}

    Cpu::Cpu(std::size_t registerCount, std::size_t floatRegisterCount, std::size_t ramSize)
            : Cpu(registerCount, floatRegisterCount, Cpu::Config::instance().aluCnt(), 2 * Cpu::Config::instance().aluCnt(), ramSize, Config::instance().ramGatesCount()) {}

    Cpu::Cpu(std::size_t registerCount, std::size_t floatRegisterCount, std::size_t aluCnt, std::size_t reservationStationEntriesCount,
        std::size_t ramSize, std::size_t ramGatesCnt)
            : registerCnt_(registerCount),
              floatRegisterCnt_(floatRegisterCount),
              physicalRegisterCnt_(specialRegistersCnt + registerCount + floatRegisterCount + reservationStationEntriesCount * possibleRenamedRegisterCnt),
              registers_(physicalRegisterCnt_),
              reservationStation_(*this, aluCnt, reservationStationEntriesCount),
              branchPredictor_{std::make_unique<NaiveBranchPredictor>()},
              rat_(*this, registerCount, floatRegisterCount),
              ram_(ramSize, ramGatesCnt)
    {
        // To be sure, theoretically not needed
        for (std::size_t i = 0; i < registerCount; ++i) {
            setRegister(Register{i}, 0);
        }
        for (std::size_t i = 0; i < floatRegisterCount; ++i) {
            setFloatRegister(FloatRegister{i}, 0);
        }
        std::fill(debug_registers_.begin(), debug_registers_.end(), 0);
        setRegister(Register::ProgramCounter(), 0);
        setRegister(Register::Flags(), 0);
        setRegister(Register::StackPointer(), ram_.size());
        setRegister(Register::StackBasePointer(), ram_.size());
    }

    void Cpu::setRegister(Register reg, int64_t value) {
        setRegister(rat_.translate(reg), value);
    }

    void Cpu::setRegisterDebug(Register reg, int64_t value) {
        setRegister(rat_.translate(reg), value);
        if (reg == Register::ProgramCounter()) {
            log_info("Setting PC!");
            speculativeProgramCounter_ = value;
        }
    }

    void Cpu::setFloatRegisterDebug(FloatRegister fReg, double value) {
        setFloatRegister(fReg, value);
    }

    void Cpu::setFloatRegister(FloatRegister fReg, double value) {
        setRegister(rat_.translate(fReg), value);
    }

    void Cpu::setRegister(PhysicalRegister reg, int64_t value) {
        registers_.at(reg.index()).value = value;
        registers_.at(reg.index()).ready = true;
    }

    void Cpu::setRegister(PhysicalRegister reg, double value) {
        registers_.at(reg.index()).value = utils::reinterpret_safe<int64_t>(value); // Store the double as int64_t
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

    int Cpu::interrupted() const {
        return interrupted_;
    }

    void Cpu::interrupt(int code) {
        interrupted_ = code;
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
        auto write = writesManager_.getWrite(id);
        writesManager_.startWriting(id, ram_);
        checkWrite(write.address());
    }

    int64_t Cpu::getMemory(uint64_t address) const {
        return ram_.get(address);
    }

    void Cpu::setMemory(uint64_t address, int64_t value) {
        ram_.set(address, value);
    }

    const Instruction& Cpu::getText(uint64_t address) {
        return program_.at(address);
    }

    void Cpu::setText(uint64_t address, std::unique_ptr<Instruction> ins) {
        program_.instructions_.at(address) = std::move(ins);
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

    bool Cpu::isTrapFlagSet() {
        // int64_t flags = getRegister(Register::Flags());
        // log_debug("flag register value: {:x}", flags);
        // return (flags & TRAP_FLAG_MASK) >> 10;
        return trapFlag_;
    }

    void Cpu::setTrapFlag() {
        // int64_t flags = getRegister(Register::Flags());
        // log_info("trap flag was set");
        // flags = flags | TRAP_FLAG_MASK;
        // setRegister(Register::Flags(), flags);
        trapFlag_ = true;
    }

    void Cpu::unsetTrapFlag() {
        // int64_t flags = getRegister(Register::Flags());
        // log_info("trap flag was unset");
        // flags = flags & ~TRAP_FLAG_MASK;
        // setRegister(Register::Flags(), flags);
        trapFlag_ = false;
    }

    bool Cpu::singleStepDone() const {
        return single_stepped_;
    }

    void Cpu::singleStepped() {
        single_stepped_ = true;
    }

    void Cpu::checkWrite(uint64_t address) {
        log_debug("Write on {}", address);
        for (size_t i = 0; i < debug_registers_.size() - 1; ++i) {
            auto& control_reg = debug_registers_[DEBUG_CONTROL_REG_IDX];
            if (!(control_reg & (1 << i))) {
                continue;
            }
            if (debug_registers_[i] == address) {
                log_info("Memory write on address '{}' where watchpoint is set", address);
                reinterpret_cast<uint8_t*>(&control_reg)[1] = 1 << i;
                interrupted_ = 2;
            }
        }
    }
}
