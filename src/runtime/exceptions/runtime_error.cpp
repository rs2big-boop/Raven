// runtime_error.cpp — Raven runtime error hierarchy
#include <string>
#include <stdexcept>

namespace raven::runtime {

class RavenException : public std::runtime_error {
public:
    explicit RavenException(const std::string& message)
        : std::runtime_error(message), message_(message) {}

    const std::string& message() const { return message_; }

private:
    std::string message_;
};

class TypeError : public RavenException {
public:
    explicit TypeError(const std::string& msg)
        : RavenException("TypeError: " + msg) {}
};

class IndexError : public RavenException {
public:
    explicit IndexError(const std::string& msg)
        : RavenException("IndexError: " + msg) {}
};

class DivisionByZeroError : public RavenException {
public:
    DivisionByZeroError()
        : RavenException("DivisionByZero: division by zero") {}
};

} // namespace raven::runtime
