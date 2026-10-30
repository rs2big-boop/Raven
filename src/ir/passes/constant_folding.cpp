#include "raven/ir/passes/constant_folding.hpp"
#include <unordered_map>

namespace raven::ir::passes {

size_t ConstantFoldingPass::run(IRFunction& fn) {
    size_t folded_count = 0;
    std::unordered_map<std::string, double> const_num_table;

    for (auto& block : fn.blocks) {
        for (auto& inst : block.instructions) {
            if (inst.op == IROpCode::ConstFloat || inst.op == IROpCode::ConstInt) {
                const_num_table[inst.dest] = inst.num_val;
            } else if (inst.args.size() == 2) {
                auto it1 = const_num_table.find(inst.args[0]);
                auto it2 = const_num_table.find(inst.args[1]);
                if (it1 != const_num_table.end() && it2 != const_num_table.end()) {
                    double a = it1->second;
                    double b = it2->second;
                    bool folded = false;
                    double res = 0.0;

                    switch (inst.op) {
                        case IROpCode::Add:      res = a + b; folded = true; break;
                        case IROpCode::Subtract: res = a - b; folded = true; break;
                        case IROpCode::Multiply: res = a * b; folded = true; break;
                        case IROpCode::Divide:
                            if (b != 0) { res = a / b; folded = true; }
                            break;
                        case IROpCode::Equal:    res = (a == b) ? 1.0 : 0.0; folded = true; break;
                        case IROpCode::NotEqual: res = (a != b) ? 1.0 : 0.0; folded = true; break;
                        case IROpCode::Less:     res = (a < b)  ? 1.0 : 0.0; folded = true; break;
                        default: break;
                    }

                    if (folded) {
                        inst.op = IROpCode::ConstFloat;
                        inst.num_val = res;
                        inst.args.clear();
                        const_num_table[inst.dest] = res;
                        folded_count++;
                    }
                }
            }
        }
    }

    return folded_count;
}

} // namespace raven::ir::passes
