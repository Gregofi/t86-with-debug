#include "register_allocation_table.h"

#include <cassert>

#include "../cpu.h"

namespace tiny::t86 {

    RegisterAllocationTable::RegisterAllocationTable(Cpu& cpu, std::size_t registerCnt, std::size_t floatRegisterCnt)
            : cpu_{cpu} {
        std::size_t i = 0;
        for (; i < registerCnt; ++i) {
            table_.insert_or_assign(Register{i}, PhysicalRegister{i});
        }
        for (std::size_t j = 0; j < floatRegisterCnt; ++j, ++i) {
            table_.insert_or_assign(FloatRegister{j}, PhysicalRegister{i});
        }
        table_.insert_or_assign(Register::ProgramCounter(), PhysicalRegister{ ++i });
        table_.insert_or_assign(Register::StackPointer(), PhysicalRegister{ ++i });
        table_.insert_or_assign(Register::StackBasePointer(), PhysicalRegister{ ++i });
        table_.insert_or_assign(Register::Flags(), PhysicalRegister{ ++i });
        subscribeToReads();
    }

    RegisterAllocationTable::RegisterAllocationTable(const RegisterAllocationTable& other)
            : table_{other.table_}, cpu_{other.cpu_} {
        subscribeToReads();
    }

    RegisterAllocationTable::RegisterAllocationTable(RegisterAllocationTable&& other)
            : table_{std::move(other.table_)}, cpu_{other.cpu_} {
    }


    RegisterAllocationTable::~RegisterAllocationTable() {
        unsubscribeFromReads();
    }

    void RegisterAllocationTable::subscribeToReads() {
        for (const auto&[x, physical] : table_) {
            cpu_.subscribeRegisterRead(physical);
        }
    }

    void RegisterAllocationTable::unsubscribeFromReads() {
        for (const auto&[x, physical] : table_) {
            cpu_.unsubscribeRegisterRead(physical);
        }
    }

    RegisterAllocationTable& RegisterAllocationTable::operator=(const RegisterAllocationTable& other) {
        assert(&cpu_ == &other.cpu_);

        if (&other == this) {
            return *this;
        }

        unsubscribeFromReads();

        table_ = other.table_;

        subscribeToReads();

        return *this;
    }

    void RegisterAllocationTable::rename(Register from, PhysicalRegister to) {
        if (auto it = table_.find(from); it != table_.end()) {
            cpu_.unsubscribeRegisterRead(it->second);
        }
        table_.insert_or_assign(from, to);
        cpu_.subscribeRegisterRead(to);
    }

    void RegisterAllocationTable::rename(FloatRegister from, PhysicalRegister to) {
        if (auto it = table_.find(from); it != table_.end()) {
            cpu_.unsubscribeRegisterRead(it->second);
        }
        table_.insert_or_assign(from, to);
        cpu_.subscribeRegisterRead(to);
    }

    PhysicalRegister RegisterAllocationTable::translate(Register reg) const {
        return table_.at(reg);
    }

    PhysicalRegister RegisterAllocationTable::translate(FloatRegister fReg) const {
        return table_.at(fReg);
    }

    bool RegisterAllocationTable::isUnmapped(PhysicalRegister reg) const {
        return std::all_of(table_.begin(), table_.end(), [reg](const auto& mapping) {
            return mapping.second.index() != reg.index();
        });
    }
}