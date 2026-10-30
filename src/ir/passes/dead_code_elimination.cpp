#include "raven/ir/passes/dead_code_elimination.hpp"
#include <unordered_set>
#include <vector>

namespace raven::ir::passes {

size_t DeadCodeEliminationPass::run(IRFunction& fn) {
    size_t eliminated_count = 0;

    // Collect all used variables
    std::unordered_set<std::string> used_vars;
    for (const auto& block : fn.blocks) {
        for (const auto& inst : block.instructions) {
            for (const auto& arg : inst.args) {
                used_vars.insert(arg);
            }
        }
    }

    // Filter out instructions whose destination is a temp and never used
    for (auto& block : fn.blocks) {
        std::vector<IRInstruction> kept;
        kept.reserve(block.instructions.size());

        for (auto& inst : block.instructions) {
            // Check if instruction produces an unused temp (starts with %)
            if (!inst.dest.empty() && inst.dest[0] == '%' && !used_vars.count(inst.dest)) {
                eliminated_count++;
                continue;
            }
            kept.push_back(std::move(inst));
        }
        block.instructions = std::move(kept);
    }

    return eliminated_count;
}

} // namespace raven::ir::passes
