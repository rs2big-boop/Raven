#include "raven/interpreter/environment.hpp"

namespace raven {

std::shared_ptr<Environment> create_global_environment() {
    return std::make_shared<Environment>();
}

std::shared_ptr<Environment> create_nested_environment(std::shared_ptr<Environment> parent) {
    return std::make_shared<Environment>(std::move(parent));
}

} // namespace raven
