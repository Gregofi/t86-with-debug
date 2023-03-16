#include "debugger/T86Process.h"


/// Writes into the processes text area, overwriting instructions.
/// You can only write up to the size of the program.
/// Caller is responsible that the text size of the program is
/// respected.
void T86Process::WriteText(uint64_t address,
                           const std::vector<std::string> &data) {
    for (size_t i = 0; i < data.size(); ++i) {
        std::istringstream iss(data[i]);
        Parser p(iss);
        try {
            p.Instruction();
            p.CheckEnd();
        } catch (const ParserError& err) {
            throw DebuggerError(fmt::format("Error in parsing instruction: {}", err.what()));
        }
        process->Send(fmt::format("POKETEXT {} {}", address + i, data[i]));
        CheckResponse("POKETEXT error");
    }
}

/// Returns range of instructions of length 'amount' starting at 'address'.
/// Caller is responsible that the size of the program is respected.
/// Use the TextSize for getting the size of text.
std::vector<std::string> T86Process::ReadText(uint64_t address, size_t amount) {
    process->Send(fmt::format("PEEKTEXT {} {}", address, amount));
    auto text = process->Receive();
    if (!text) {
        throw DebuggerError("PEEKTEXT err");
    }

    return utils::split(*text, '\n');
}

void T86Process::WriteMemory(uint64_t address, const std::vector<int64_t>& data) {
    if (address + data.size() > data_size) {
        throw DebuggerError(fmt::format("Writing memory at {}-{}, but size is {}",
                    address, address + data.size() - 1, data_size));
    }
    for (size_t i = 0; i < data.size(); ++i) {
        process->Send(fmt::format("POKEDATA {} {}", address + i, data[i]));
        CheckResponse("POKEDATA error");
    }
}

std::vector<int64_t> T86Process::ReadMemory(uint64_t address, size_t amount) {
    if (address + amount > data_size) {
        throw DebuggerError(fmt::format("Reading memory at {}-{}, but size is {}",
                    address, address + amount - 1, data_size));
    }
    process->Send(fmt::format("PEEKDATA {} {}", address, amount));
    auto data = process->Receive();
    if (!data) {
        throw DebuggerError("PEEKDATA err");
    }

    auto splitted = utils::split(*data, '\n');
    std::vector<int64_t> result;
    std::transform(splitted.begin(), splitted.end(), std::back_inserter(result),
            [](auto&& s) { return std::stoll(s); });
    return result;
}

StopReason T86Process::GetReason() {
    process->Send("REASON");
    auto reason_opt = process->Receive();
    if (!reason_opt) {
        throw DebuggerError("REASON error");
    }
    auto &r = *reason_opt;
    if (r == "START") {
        return StopReason::ExecutionBegin;
    } else if (r == "SW_BKPT") {
        return StopReason::SoftwareBreakpointHit;
    } else if (r == "HW_BKPT") {
        return StopReason::HardwareBreak;
    } else if (r == "SINGLESTEP") {
        return StopReason::Singlestep;
    } else if (r == "HALT") {
        return StopReason::ExecutionEnd;
    } else {
        UNREACHABLE;
    }
}

void T86Process::Singlestep() {
    process->Send("SINGLESTEP");
    CheckResponse("SINGLESTEP error");
}

std::map<std::string, int64_t> T86Process::FetchRegisters() {
    process->Send("PEEKREGS");
    return FetchRegistersOfType<int64_t>();
}

void T86Process::SetRegisters(const std::map<std::string, int64_t>& regs) {
    for (const auto& [name, val]: regs) {
        if (!IsValidRegisterName(name)) {
            throw DebuggerError(fmt::format("Register name '{}' is not valid!", name));
        }
        log_info("Setting register {} to value {}", name, val);
        process->Send(fmt::format("POKEREGS {} {}", name, val));
        CheckResponse("POKEREGS error");
    }
}

std::map<std::string, double> T86Process::FetchFloatRegisters() {
    process->Send("PEEKFLOATREGS");
    return FetchRegistersOfType<double>();
}

void T86Process::SetFloatRegisters(const std::map<std::string, double>& regs) {
    for (const auto& [name, val]: regs) {
        if (!IsValidFloatRegisterName(name)) {
            throw DebuggerError(fmt::format("Register name '{}' is not valid!", name));
        }
        log_info("Setting register {} to value {}", name, val);
        process->Send(fmt::format("POKEFLOATREGS {} {}", name, val));
        CheckResponse("POKEFLOATREGS error");
    }
}

std::map<std::string, uint64_t> T86Process::FetchDebugRegisters() {
    process->Send("PEEKDEBUGREGS");
    return FetchRegistersOfType<uint64_t>();
}

void T86Process::SetDebugRegisters(const std::map<std::string, uint64_t>& regs) {
    for (const auto& [name, val]: regs) {
        if (!IsValidDebugRegisterName(name)) {
            throw DebuggerError(fmt::format("Register name '{}' is not valid!", name));
        }
        log_debug("sending: `POKEDEBUGREGS {} {}`", name, val);
        process->Send(fmt::format("POKEDEBUGREGS {} {}", name, val));
        CheckResponse("POKEDEBUGREGS error");
    }
}

size_t T86Process::TextSize() {
    process->Send("TEXTSIZE");
    auto response = process->Receive();
    if (!response) {
        throw DebuggerError("TEXTSIZE error");
    }
    size_t result = std::stoull(response->c_str() + strlen("TEXTSIZE:"));
    return result;
}

void T86Process::ResumeExecution() {
    process->Send("CONTINUE");
    CheckResponse("CONTINUE fail");
}

void T86Process::Wait() {
    auto message = process->Receive();
    if (!message || message != "STOPPED") {
        throw DebuggerError(fmt::format("Expected STOPPED message in Wait()"));
    }
}

void T86Process::Terminate() {
    process->Send("TERMINATE");
    CheckResponse("TERMINATE fail");
}

int64_t T86Process::GetRegister(std::string_view name) {
    process->Send(fmt::format("PEEKREGS {}", name));
    auto response = process->Receive();
    if (!response) {
        throw DebuggerError("PEEKREGS error");
    }
    auto offset = response->find("VALUE:") + strlen("VALUE:");
    auto str_val = response->substr(offset);
    auto value = std::stoll(response->substr(offset));
    return value;
}

bool T86Process::isGPRegister(std::string_view name) const {
    if (name.size() >= 2 
            && name[0] == 'R') {
        auto idx = utils::svtonum<size_t>(name.substr(1));
        return idx && 0 <= *idx && *idx < gen_purpose_regs_count;
    }
    return false;
}

bool T86Process::IsValidRegisterName(std::string_view name) const {
    return name == "IP"
           || name == "BP"
           || name == "SP"
           || name == "FLAGS"
           || isGPRegister(name);
}

bool T86Process::IsValidFloatRegisterName(std::string_view name) const {
    if (name.size() >= 2
            && name[0] == 'F') {
        auto idx = utils::svtonum<size_t>(name.substr(1));
        return idx && 0 <= *idx && *idx < float_regs_count;
    }
    return false;
}

bool T86Process::IsValidDebugRegisterName(std::string_view name) const {
    if (name.size() >= 2 
            && name[0] == 'D') {
        auto idx = utils::svtonum<size_t>(name.substr(1));
        return idx && 0 <= *idx && *idx < debug_register_cnt;
    }
    return false;
}

void T86Process::CheckResponse(std::string_view error_message) {
    auto message = process->Receive();
    if (!message || *message != "OK") {
        auto report = fmt::format("Error communicating with T86 VM: {}", error_message);
        if (message) {
            report += fmt::format("; Expected 'OK', got '{}'", *message);
        } else {
            report += fmt::format("; No confirmation was sent back");
        }
        throw DebuggerError(report);
    }
}
