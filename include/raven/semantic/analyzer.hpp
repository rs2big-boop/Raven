#pragma once
#include "resolver.hpp"
#include "raven/ast.hpp"
#include <vector>
#include <iostream>

namespace raven::semantic {

class Analyzer {
public:
    bool analyze(const std::vector<std::shared_ptr<Stmt>>& program);

    const std::vector<ResolverError>& errors() const { return resolver_.errors(); }
    bool has_errors() const { return resolver_.has_errors(); }

    void print_errors(std::ostream& out = std::cerr) const;

private:
    Resolver resolver_;
};

} // namespace raven::semantic
