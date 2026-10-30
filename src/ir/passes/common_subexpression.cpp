#include "raven/ir/passes/common_subexpression.hpp"
#include <unordered_map>

namespace raven::ir::passes {

size_t CommonSubexpressionPass::run(IRFunction& fn) {
    size_t eliminated_count = 0;

    for (auto& block : fn.blocks) {
        // expression signature -> dest temp
        std::unordered_map<std::string, std::string> expr_table;
        // replacement map for subsequent uses
        std::unordered_map<std::string, std::string> replacements;

        for (auto& inst : block.instructions) {
            // Apply known replacements to args
            for (auto& arg : inst.args) {
                auto rep_it = replacements.find(arg);
                if (rep_it != replacements.end()) {
                    arg = rep_it->second;
                }
            }

            // Only consider pure binary operations
            if (inst.args.size() == 2 &&
                (inst.op == IROpCode::Add || inst.op == IROpCode::Subtract ||
                 inst.op == IROpCode::Multiply || inst.op == IROpCode::Divide)) {
                std::string sig = ir_opcode_name(inst.op) + " " + inst.args[0] + " " + inst.args[1];
                auto it = expr_table.find(sig);
                if (it != expr_table.end()) {
                    // Redundant computation! Replace with load / alias
                    replacements[inst.dest] = it->second;
                    inst.op = IROpCode::LoadVar;
                    inst.args = {it->second};
                    eliminated_count++;
                } else {
                    expr_table[sig] = inst.dest;
                }
            }
        }
    }

    return eliminated_count;
}

} // namespace raven::ir::passes
