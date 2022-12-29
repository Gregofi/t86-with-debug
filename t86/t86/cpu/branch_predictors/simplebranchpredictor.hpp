#pragma once

#include "../branchpredictor.h"

#include <array>
#include <algorithm>
#include <cassert>

namespace tiny::t86 {
    // TODO in future
    /*
    template<size_t N>
    class SimpleBranchPredictor : public BranchPredictor {
        using branches = std::list<std::optional<size_t>>;
        std::map<size_t, branches> records_;
      public:
        SimpleBranchPredictor() {}


        size_t nextGuess(size_t pc, const Instruction& instruction) const override {
            auto it = records_.find(pc);
            if (it == records_.end()) {
                return false;
            }
            const branches& branches_ = it->second;
            size_t falseCnt = std::count_if(branches_.begin(), branches_.end(), [](const std::optional<size_t>& branch) {
                return !branch.has_value();
            });

            size_t trueCnt = std::count_if(branches_.begin(), branches_.end(), [](const std::optional<size_t>& branch) {
                return branch.has_value();
            });

            assert(trueCnt + falseCnt == branches_.size());
            return (trueCnt >= falseCnt);
        }

        void registerBranchTaken(size_t pc, const Instruction&, size_t destination) override {
            auto& branches_ = records_[pc];
            while(branches_.size() >= N) {
                branches_.pop_front();
            }
            branches_.emplace_back(destination);
        }

        void registerBranchNotTaken(size_t pc, const Instruction&) override {
            auto& branches_ = records_[pc];
            while(branches_.size() >= N) {
                branches_.pop_front();
            }
            branches_.emplace_back(std::nullopt);
        }
    };
     */
}
