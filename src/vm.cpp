#include "raven/vm.hpp"

namespace raven {

VM::VM(std::ostream& out) : out_(out) {}

void VM::push(Value value) {
    stack_.push_back(std::move(value));
}

Value VM::pop() {
    Value top = stack_.back();
    stack_.pop_back();
    return top;
}

Value VM::peek(int distance) const {
    return stack_[stack_.size() - 1 - distance];
}

InterpretResult VM::run(const Chunk& chunk) {
    size_t ip = 0;

    auto read_byte = [&]() -> uint8_t {
        return chunk.code[ip++];
    };

    auto read_short = [&]() -> uint16_t {
        ip += 2;
        return (static_cast<uint16_t>(chunk.code[ip - 2]) << 8) | chunk.code[ip - 1];
    };

    auto read_constant = [&]() -> Value {
        return chunk.constants[read_byte()];
    };

    while (ip < chunk.code.size()) {
        auto instruction = static_cast<OpCode>(read_byte());

        switch (instruction) {
            case OpCode::Constant: {
                push(read_constant());
                break;
            }
            case OpCode::Nil: push(Value()); break;
            case OpCode::True: push(Value(true)); break;
            case OpCode::False: push(Value(false)); break;

            case OpCode::Add: {
                Value b = pop();
                Value a = pop();
                if (a.is_number() && b.is_number()) {
                    push(Value(a.as_number() + b.as_number()));
                } else if (a.is_string() || b.is_string()) {
                    push(Value(a.to_string() + b.to_string()));
                } else if (a.is_array() && b.is_array()) {
                    auto res = std::make_shared<std::vector<Value>>(*a.as_array());
                    res->insert(res->end(), b.as_array()->begin(), b.as_array()->end());
                    push(Value(res));
                } else {
                    out_ << "Runtime error: operands must be numbers or strings.\n";
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::Subtract: {
                Value b = pop();
                Value a = pop();
                push(Value(a.as_number() - b.as_number()));
                break;
            }
            case OpCode::Multiply: {
                Value b = pop();
                Value a = pop();
                push(Value(a.as_number() * b.as_number()));
                break;
            }
            case OpCode::Divide: {
                Value b = pop();
                Value a = pop();
                if (b.as_number() == 0.0) {
                    out_ << "Runtime error: division by zero.\n";
                    return InterpretResult::RuntimeError;
                }
                push(Value(a.as_number() / b.as_number()));
                break;
            }
            case OpCode::Modulo: {
                Value b = pop();
                Value a = pop();
                if (b.as_number() == 0.0) {
                    out_ << "Runtime error: modulo by zero.\n";
                    return InterpretResult::RuntimeError;
                }
                push(Value(std::fmod(a.as_number(), b.as_number())));
                break;
            }
            case OpCode::Equal: {
                Value b = pop();
                Value a = pop();
                push(Value(a == b));
                break;
            }
            case OpCode::NotEqual: {
                Value b = pop();
                Value a = pop();
                push(Value(a != b));
                break;
            }
            case OpCode::Greater: {
                Value b = pop();
                Value a = pop();
                push(Value(a.as_number() > b.as_number()));
                break;
            }
            case OpCode::GreaterEqual: {
                Value b = pop();
                Value a = pop();
                push(Value(a.as_number() >= b.as_number()));
                break;
            }
            case OpCode::Less: {
                Value b = pop();
                Value a = pop();
                push(Value(a.as_number() < b.as_number()));
                break;
            }
            case OpCode::LessEqual: {
                Value b = pop();
                Value a = pop();
                push(Value(a.as_number() <= b.as_number()));
                break;
            }
            case OpCode::Not:
                push(Value(!pop().is_truthy()));
                break;
            case OpCode::Negate:
                push(Value(-pop().as_number()));
                break;
            case OpCode::Print: {
                Value val = pop();
                out_ << val.to_string() << "\n";
                break;
            }
            case OpCode::Pop:
                pop();
                break;
            case OpCode::DefineGlobal: {
                std::string name = read_constant().as_string();
                globals_[name] = pop();
                break;
            }
            case OpCode::GetGlobal: {
                std::string name = read_constant().as_string();
                auto it = globals_.find(name);
                if (it == globals_.end()) {
                    out_ << "Runtime error: undefined variable '" << name << "'.\n";
                    return InterpretResult::RuntimeError;
                }
                push(it->second);
                break;
            }
            case OpCode::SetGlobal: {
                std::string name = read_constant().as_string();
                auto it = globals_.find(name);
                if (it == globals_.end()) {
                    out_ << "Runtime error: undefined variable '" << name << "'.\n";
                    return InterpretResult::RuntimeError;
                }
                it->second = peek(0);
                break;
            }
            case OpCode::GetLocal: {
                uint8_t slot = read_byte();
                size_t base = frames_.empty() ? 0 : frames_.back().slots;
                push(stack_[base + slot]);
                break;
            }
            case OpCode::SetLocal: {
                uint8_t slot = read_byte();
                size_t base = frames_.empty() ? 0 : frames_.back().slots;
                stack_[base + slot] = peek(0);
                break;
            }
            case OpCode::JumpIfFalse: {
                uint16_t offset = read_short();
                if (!peek(0).is_truthy()) {
                    ip += offset;
                }
                break;
            }
            case OpCode::Jump: {
                uint16_t offset = read_short();
                ip += offset;
                break;
            }
            case OpCode::Loop: {
                uint16_t offset = read_short();
                ip -= offset;
                break;
            }
            case OpCode::BuildArray: {
                uint8_t count = read_byte();
                auto list = std::make_shared<std::vector<Value>>();
                list->resize(count);
                for (int i = count - 1; i >= 0; --i) {
                    (*list)[i] = pop();
                }
                push(Value(list));
                break;
            }
            case OpCode::IndexGet: {
                Value idx = pop();
                Value obj = pop();
                if (obj.is_array()) {
                    int i = static_cast<int>(idx.as_number());
                    const auto& arr = *obj.as_array();
                    if (i < 0) i += static_cast<int>(arr.size());
                    push(arr[i]);
                }
                break;
            }
            case OpCode::IndexSet: {
                Value val = pop();
                Value idx = pop();
                Value obj = pop();
                if (obj.is_array()) {
                    int i = static_cast<int>(idx.as_number());
                    auto& arr = *obj.as_array();
                    if (i < 0) i += static_cast<int>(arr.size());
                    arr[i] = val;
                    push(val);
                }
                break;
            }
            case OpCode::Call: {
                uint8_t arg_count = read_byte();
                Value callee = peek(arg_count);
                if (!callee.is_callable()) {
                    out_ << "Runtime error: can only call functions.\n";
                    return InterpretResult::RuntimeError;
                }
                auto* fn = dynamic_cast<BytecodeFunction*>(callee.as_callable().get());
                if (fn) {
                    if (fn->arity() != arg_count) {
                        out_ << "Runtime error: expected " << fn->arity() << " arguments but got " << static_cast<int>(arg_count) << ".\n";
                        return InterpretResult::RuntimeError;
                    }
                    if (frames_.size() > 1000) {
                        out_ << "Runtime error: call stack overflow.\n";
                        return InterpretResult::RuntimeError;
                    }
                    CallFrame frame;
                    frame.return_ip = ip;
                    frame.slots = stack_.size() - arg_count;
                    frames_.push_back(frame);
                    ip = fn->entry_ip();
                } else {
                    out_ << "Runtime error: unsupported callable in VM.\n";
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OpCode::Return: {
                Value result = stack_.empty() ? Value() : pop();
                if (frames_.empty()) {
                    return InterpretResult::Ok;
                }
                CallFrame frame = frames_.back();
                frames_.pop_back();

                size_t target_size = frame.slots > 0 ? frame.slots - 1 : 0;
                while (stack_.size() > target_size) {
                    stack_.pop_back();
                }
                push(result);
                ip = frame.return_ip;
                break;
            }
        }
    }

    return InterpretResult::Ok;
}

} // namespace raven
