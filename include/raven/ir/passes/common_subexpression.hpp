#pragma once
#include "raven/ir/ir_node.hpp"

namespace raven::ir::passes {

class CommonSubexpressionPass {
public:
    static size_t run(IRFunction& fn);
};

} // namespace raven::ir::passes
