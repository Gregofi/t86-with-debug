
#include <functional>
#include <iterator>
#include <vector>
#include <iostream>
#include <optional>
#include <ranges>
#include <algorithm>
#include <fmt/core.h>

#include "common/helpers.h"
#include "common/logger.h"
#include "tinyC/backend/operand.h"
#include "tinyC/backend/register_allocator.h"

Register tinyc::InfinityRegisterAllocator::getRegister(size_t value_id) {
    used_registers.emplace(value_id);
    return Register{value_id};
}

std::set<Register> tinyc::InfinityRegisterAllocator::registersInUse() const {
    std::set<Register> result;
    std::transform(used_registers.begin(), used_registers.end(), std::inserter(result, result.begin()),
                           [](const auto& x) { return Register(x); });
    return result;
}

/* ================================================================== */

namespace tinyc {
    std::optional<LinearRegisterAllocator::reg_id> LinearRegisterAllocator::RegisterManager::fetchFreeRegister() const {
        auto it = std::find(active_registers.begin(), active_registers.end(), false);
        if (it == active_registers.end()) {
            return std::nullopt;
        } else {
            return std::distance(active_registers.begin(), it);
        }
    }

    void LinearRegisterAllocator::RegisterManager::updateRegisterUse(tinyc::LinearRegisterAllocator::reg_id id) {
        auto it = std::find(register_usage.begin(), register_usage.end(), id);
        active_registers[id] = true;
        if (it != register_usage.end()) {
            register_usage.erase(it);
        }
        register_usage.push_back(id);
    }

    LinearRegisterAllocator::reg_id tinyc::LinearRegisterAllocator::RegisterManager::spillRegister() {
        std::cout << std::flush;
        assert(!register_usage.empty());

        // Find the most old register
        reg_id to_free = register_usage.front();
        register_usage.pop_front();

        Logger::log("Spilling register {}.\n", to_free);

        // Find the value which register stores
        auto it = std::find_if(register_mapping.begin(), register_mapping.end(), [to_free](const auto& it) { return it.second == to_free; });
        assert(it != register_mapping.end());

        // Write the register contents to memory
        memory_manager.storeRegToMem(to_free, it->first);
        register_mapping.erase(it);

        return to_free;
    }

    std::set<Register> LinearRegisterAllocator::RegisterManager::registersInUse() const {
        std::set<Register> res;
        for (size_t i = 0; i < active_registers.size(); ++i) {
            if (active_registers[i]) {
                res.insert(Register{i});
            }
        }
        return res;
    }

    LinearRegisterAllocator::memory_offset LinearRegisterAllocator::SpilledManager::fetchFreeMemoryCell() const {
        auto it = std::find(taken_memory.begin(), taken_memory.end(), false);
        if (it == taken_memory.end()) {
            throw std::runtime_error("Register allocator run out of memory");
        }
        return std::distance(taken_memory.begin(), it);
    }

    bool LinearRegisterAllocator::SpilledManager::isSpilled(LinearRegisterAllocator::value_id val) const {
        return spilled.contains(val);
    }

    std::optional<LinearRegisterAllocator::reg_id>
    LinearRegisterAllocator::RegisterManager::getRegisterId(LinearRegisterAllocator::value_id value) const {
        auto it = register_mapping.find(value);
        if (it == register_mapping.end()) {
            return std::nullopt;
        } else {
            return it->second;
        }
    }

    void LinearRegisterAllocator::SpilledManager::storeMemToReg(LinearRegisterAllocator::value_id val,
                                                                LinearRegisterAllocator::reg_id reg) {
        auto memory_it = spilled.find(val);
        if (memory_it == spilled.end()) {
            throw std::runtime_error("Value is not in spilled DB.");
        }
        // builder.add(tiny::t86::MOV{Register(reg), tiny::t86::Mem(memory_it->second)});
        program.add_ins(T86Ins::Opcode::MOV, Register{reg}, Memory{memory_it->second});
        taken_memory[memory_it->second] = false;
        spilled.erase(memory_it);
    }

    LinearRegisterAllocator::memory_offset
    LinearRegisterAllocator::SpilledManager::storeRegToMem(LinearRegisterAllocator::reg_id to_free,
                                                           LinearRegisterAllocator::value_id freed_value) {
        memory_offset free_cell = fetchFreeMemoryCell();
        Logger::log("Storing register {} with value {} to memory cell {}.\n", to_free, freed_value, free_cell);
        program.add_ins(T86Ins::Opcode::MOV, Memory{free_cell}, Register{to_free});
        taken_memory[free_cell] = true;
        spilled[freed_value] = free_cell;
        return free_cell;
    }

    Register LinearRegisterAllocator::getRegister(value_id value_id) {
        // Has 4 cases
        // 1. The register is allocated - return the register.
        // 2. Check if the register is spilled - if so, unspill it and return it.
        // 3. The register does not exist - If no register is available, unspill
        // one and return it.
        // 4. Return free register.

        // 1. - The register is allocated
        reg_id reg_result;
        if (auto reg_id = reg_manager.getRegisterId(value_id)) {
            Logger::log("Fetching value {} which is in register {}\n", value_id, *reg_id);

            reg_result = *reg_id;
        }
        // 2. - The register is spilled
        else if (spilled_manager.isSpilled(value_id)) {
            Logger::log("Fetching value {} is which is spilled.\n", value_id);

            int free_reg;
            /* Try to fetch free reg, if none is available spill one */
            if (auto r = reg_manager.fetchFreeRegister()) {
                free_reg = *r;
            } else {
                free_reg = reg_manager.spillRegister();
            }

            spilled_manager.storeMemToReg(value_id, free_reg);
            reg_result = free_reg;
        // The value is new - Allocate either register or memory for it
        } else {
            // 3. - Return free register.
            auto free_reg = reg_manager.fetchFreeRegister();
            if (free_reg) {
                Logger::log("Allocating new register {} for value {}.\n", *free_reg, value_id);

                reg_result = *free_reg;
            // 4. - Spill some existing register
            } else {
                auto spilled = reg_manager.spillRegister();
                Logger::log("Allocating new value {}: Spilling existing register {} to make space.\n", value_id, spilled);

                reg_result = spilled;
            }
        }
        reg_manager.updateRegisterUse(reg_result);
        reg_manager.matchValToReg(value_id, reg_result);
        return Register{reg_result};
    }
    Register LinearRegisterAllocator::getAX() const {
        return Register(MAX_REGISTERS);
    }

    std::set<Register> LinearRegisterAllocator::registersInUse() const {
        return reg_manager.registersInUse();
    }

    LinearRegisterAllocator::LinearRegisterAllocator(std::map<size_t, Interval> ins_ranges, Program& program, size_t reg_count)
        : MAX_REGISTERS(reg_count), ins_ranges(std::move(ins_ranges)),
          program(program), spilled_manager{program}, reg_manager {program,spilled_manager, MAX_REGISTERS} {

          }
    void LinearRegisterAllocator::RegisterManager::matchValToReg(value_id val, reg_id reg) {
        register_mapping[val] = reg;
    }
} // namespace tinyc
