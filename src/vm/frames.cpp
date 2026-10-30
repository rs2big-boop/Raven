// frames.cpp — Call frame management for the Corvus VM
#include <cstddef>

namespace raven::vm {

struct CallFrameDescriptor {
    size_t return_ip;
    size_t slots;
};

} // namespace raven::vm
