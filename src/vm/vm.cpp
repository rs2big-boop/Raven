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
            case OpCode::JumpIfNotNil: {
                uint16_t offset = read_short();
                if (!peek(0).is_nil()) {
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
            case OpCode::BuildMap: {
                uint8_t count = read_byte();
                auto map = std::make_shared<std::unordered_map<std::string, Value>>();
                for (int i = 0; i < count; ++i) {
                    Value val = pop();
                    Value key = pop();
                    (*map)[key.to_string()] = val;
                }
                push(Value(map));
                break;
            }
            case OpCode::IndexGet: {
                Value idx = pop();
                Value obj = pop();
                if (obj.is_array()) {
                    int i = static_cast<int>(idx.as_number());
                    const auto& arr = *obj.as_array();
                    if (i < 0) i += static_cast<int>(arr.size());
                    if (i >= 0 && i < static_cast<int>(arr.size())) {
                        push(arr[i]);
                    } else {
                        push(Value());
                    }
                } else if (obj.is_map()) {
                    const auto& map = *obj.as_map();
                    std::string key = idx.to_string();
                    auto it = map.find(key);
                    if (it != map.end()) {
                        push(it->second);
                    } else {
                        push(Value());
                    }
                } else if (obj.is_string()) {
                    int i = static_cast<int>(idx.as_number());
                    const auto& str = obj.as_string();
                    if (i < 0) i += static_cast<int>(str.size());
                    if (i >= 0 && i < static_cast<int>(str.size())) {
                        push(Value(std::string(1, str[i])));
                    } else {
                        push(Value());
                    }
                } else {
                    push(Value());
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
                    if (i >= 0 && i < static_cast<int>(arr.size())) {
                        arr[i] = val;
                    }
                    push(val);
                } else if (obj.is_map()) {
                    auto& map = *obj.as_map();
                    map[idx.to_string()] = val;
                    push(val);
                } else {
                    push(val);
                }
                break;
            }
            case OpCode::Length: {
                Value val = pop();
                if (val.is_array()) {
                    push(Value(static_cast<double>(val.as_array()->size())));
                } else if (val.is_map()) {
                    push(Value(static_cast<double>(val.as_map()->size())));
                } else if (val.is_string()) {
                    push(Value(static_cast<double>(val.as_string().size())));
                } else {
                    push(Value(0.0));
                }
                break;
            }
            case OpCode::ArrayPush: {
                Value val = pop();
                Value arr = pop();
                if (arr.is_array()) {
                    arr.as_array()->push_back(val);
                    push(Value());
                } else {
                    push(Value());
                }
                break;
            }
            case OpCode::ArrayPop: {
                Value arr = pop();
                if (arr.is_array() && !arr.as_array()->empty()) {
                    Value last = arr.as_array()->back();
                    arr.as_array()->pop_back();
                    push(last);
                } else {
                    push(Value());
                }
                break;
            }
            case OpCode::ArrayReverse: {
                Value arr = pop();
                if (arr.is_array()) {
                    auto result = std::make_shared<std::vector<Value>>(
                        arr.as_array()->rbegin(), arr.as_array()->rend());
                    push(Value(result));
                } else {
                    push(arr);
                }
                break;
            }
            case OpCode::MapKeys: {
                Value map = pop();
                auto result = std::make_shared<std::vector<Value>>();
                if (map.is_map()) {
                    for (auto& [k, v] : *map.as_map()) {
                        result->push_back(Value(k));
                    }
                }
                push(Value(result));
                break;
            }
            case OpCode::MapValues: {
                Value map = pop();
                auto result = std::make_shared<std::vector<Value>>();
                if (map.is_map()) {
                    for (auto& [k, v] : *map.as_map()) {
                        result->push_back(v);
                    }
                }
                push(Value(result));
                break;
            }
            case OpCode::MapHasKey: {
                Value key = pop();
                Value map = pop();
                if (map.is_map()) {
                    push(Value(map.as_map()->count(key.to_string()) > 0));
                } else {
                    push(Value(false));
                }
                break;
            }
            case OpCode::TypeOf: {
                Value val = pop();
                std::string type_str;
                if (val.is_nil()) type_str = "nil";
                else if (val.is_bool()) type_str = "bool";
                else if (val.is_number()) type_str = "number";
                else if (val.is_string()) type_str = "string";
                else if (val.is_array()) type_str = "array";
                else if (val.is_map()) type_str = "map";
                else if (val.is_callable()) type_str = "function";
                else type_str = "unknown";
                push(Value(type_str));
                break;
            }
            case OpCode::ToString: {
                Value val = pop();
                push(Value(val.to_string()));
                break;
            }
            case OpCode::ToNumber: {
                Value val = pop();
                if (val.is_number()) {
                    push(val);
                } else if (val.is_string()) {
                    try {
                        push(Value(std::stod(val.as_string())));
                    } catch (...) {
                        push(Value(0.0));
                    }
                } else if (val.is_bool()) {
                    push(Value(val.as_bool() ? 1.0 : 0.0));
                } else {
                    push(Value(0.0));
                }
                break;
            }
            case OpCode::Slice: {
                Value end_val = pop();
                Value start_val = pop();
                Value obj = pop();

                if (obj.is_array()) {
                    auto& arr = *obj.as_array();
                    int sz = static_cast<int>(arr.size());
                    int start = start_val.is_nil() ? 0 : static_cast<int>(start_val.as_number());
                    int end = end_val.is_nil() ? sz : static_cast<int>(end_val.as_number());
                    if (start < 0) start = std::max(0, sz + start);
                    if (end < 0) end = std::max(0, sz + end);
                    start = std::max(0, std::min(start, sz));
                    end = std::max(start, std::min(end, sz));
                    auto result = std::make_shared<std::vector<Value>>(
                        arr.begin() + start, arr.begin() + end);
                    push(Value(result));
                } else if (obj.is_string()) {
                    const std::string& s = obj.as_string();
                    int sz = static_cast<int>(s.size());
                    int start = start_val.is_nil() ? 0 : static_cast<int>(start_val.as_number());
                    int end = end_val.is_nil() ? sz : static_cast<int>(end_val.as_number());
                    if (start < 0) start = std::max(0, sz + start);
                    if (end < 0) end = std::max(0, sz + end);
                    start = std::max(0, std::min(start, sz));
                    end = std::max(start, std::min(end, sz));
                    push(Value(s.substr(start, end - start)));
                } else {
                    push(Value());
                }
                break;
            }
            case OpCode::CheckType: {
                uint8_t const_idx = read_byte();
                std::string type_name = chunk.constants[const_idx].to_string();
                for (char& c : type_name) c = std::tolower(static_cast<unsigned char>(c));

                Value val = pop();
                bool match = false;
                if (type_name == "number" || type_name == "num") match = val.is_number();
                else if (type_name == "string" || type_name == "str") match = val.is_string();
                else if (type_name == "bool" || type_name == "boolean") match = val.is_bool();
                else if (type_name == "array" || type_name == "list") match = val.is_array();
                else if (type_name == "map" || type_name == "dict") match = val.is_map();
                else if (type_name == "nil" || type_name == "null") match = val.is_nil();
                else if (type_name == "function" || type_name == "fn" || type_name == "callable") match = val.is_callable();
                else if (val.is_map()) {
                    auto m = val.as_map();
                    auto it = m->find("__type__");
                    if (it != m->end() && it->second.to_string() == type_name) match = true;
                }
                push(Value(match));
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
