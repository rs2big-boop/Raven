#include <gtest/gtest.h>
#include "raven/ir/ir_node.hpp"
#include "raven/ir/ir_builder.hpp"
#include "raven/ir/passes/constant_folding.hpp"
#include "raven/ir/passes/dead_code_elimination.hpp"
#include "raven/ir/passes/common_subexpression.hpp"

using namespace raven::ir;

TEST(IRTest, IRBuilderBasicInstructions) {
    IRFunction fn;
    fn.name = "test_func";

    IRBuilder builder;
    builder.set_current_function(&fn);

    std::string t0 = builder.emit_const_number(42.0);
    std::string t1 = builder.emit_const_number(8.0);
    std::string t2 = builder.emit_binop(IROpCode::Add, t0, t1);
    builder.emit_return(t2);

    ASSERT_EQ(fn.blocks.size(), 1u);
    const auto& insts = fn.blocks[0].instructions;
    ASSERT_EQ(insts.size(), 4u);
    EXPECT_EQ(insts[0].op, IROpCode::ConstFloat);
    EXPECT_EQ(insts[0].num_val, 42.0);
    EXPECT_EQ(insts[1].op, IROpCode::ConstFloat);
    EXPECT_EQ(insts[1].num_val, 8.0);
    EXPECT_EQ(insts[2].op, IROpCode::Add);
    EXPECT_EQ(insts[3].op, IROpCode::Return);
}

TEST(IRTest, ConstantFoldingPass) {
    IRFunction fn;
    fn.name = "fold_func";

    IRBuilder builder;
    builder.set_current_function(&fn);

    std::string t0 = builder.emit_const_number(10.0);
    std::string t1 = builder.emit_const_number(5.0);
    std::string t2 = builder.emit_binop(IROpCode::Multiply, t0, t1);
    builder.emit_return(t2);

    size_t folded = passes::ConstantFoldingPass::run(fn);
    EXPECT_EQ(folded, 1u);

    // The Multiply should be folded into ConstFloat with value 50.0
    const auto& insts = fn.blocks[0].instructions;
    EXPECT_EQ(insts[2].op, IROpCode::ConstFloat);
    EXPECT_EQ(insts[2].num_val, 50.0);
}

TEST(IRTest, DeadCodeEliminationPass) {
    IRFunction fn;
    fn.name = "dce_func";

    IRBuilder builder;
    builder.set_current_function(&fn);

    std::string t0 = builder.emit_const_number(100.0); // unused!
    std::string t1 = builder.emit_const_number(200.0); // used
    builder.emit_return(t1);

    size_t eliminated = passes::DeadCodeEliminationPass::run(fn);
    EXPECT_EQ(eliminated, 1u);

    const auto& insts = fn.blocks[0].instructions;
    ASSERT_EQ(insts.size(), 2u);
    EXPECT_EQ(insts[0].dest, t1);
    EXPECT_EQ(insts[1].op, IROpCode::Return);
}

TEST(IRTest, CommonSubexpressionPass) {
    IRFunction fn;
    fn.name = "cse_func";

    IRBuilder builder;
    builder.set_current_function(&fn);

    std::string a = "a";
    std::string b = "b";
    std::string t0 = builder.emit_binop(IROpCode::Add, a, b);
    std::string t1 = builder.emit_binop(IROpCode::Add, a, b); // Redundant identical expression!
    std::string t2 = builder.emit_binop(IROpCode::Multiply, t0, t1);
    builder.emit_return(t2);

    size_t eliminated = passes::CommonSubexpressionPass::run(fn);
    EXPECT_EQ(eliminated, 1u);

    const auto& insts = fn.blocks[0].instructions;
    // Second add is transformed into LoadVar referencing t0
    EXPECT_EQ(insts[1].op, IROpCode::LoadVar);
    EXPECT_EQ(insts[1].args[0], t0);
}
