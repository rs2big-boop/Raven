#pragma once
#include "raven/ir/ir_node.hpp"

namespace raven::ir::passes {

class ConstantFoldingPass {
public:
    static size_t run(IRFunction& fn);
};

} // namespace raven::ir::passes
