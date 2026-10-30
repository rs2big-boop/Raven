# Raven Syntax Guide

Raven provides expressive, modern syntax inspired by Python ergonomics and Go simplicity.

---

## Variables and Declarations

Variables are declared with `let`:
```raven
let count = 42;
let title = "Raven Programming Language";
let is_active = true;
let nullable_val = nil;
```

---

## Operators

### Arithmetic & Comparison
```raven
let sum = 10 + 5;
let diff = 10 - 5;
let product = 10 * 5;
let quotient = 10 / 5;
let remainder = 10 % 3;

let is_equal = (sum == 15);
let is_greater = (sum > 10);
```

### Null Coalescing (`??`)
Returns the left side if non-nil, otherwise evaluates the right side:
```raven
let config_port = env("PORT") ?? 8080;
```

### Optional Chaining (`?.`)
Safely traverses nested maps and properties without runtime exceptions:
```raven
let city = user?.address?.city;
let first_item = data?.items?.[0];
```

### Spread Operator (`...`)
Inlines arrays and maps:
```raven
let base_nums = [1, 2, 3];
let all_nums = [...base_nums, 4, 5];

let base_map = {"a": 1, "b": 2};
let merged_map = {...base_map, "c": 3};
```

---

## Control Flow

### If / Else
```raven
if (status == 200) {
    print("OK");
} else if (status == 404) {
    print("Not Found");
} else {
    print("Error");
}
```

### While Loop
```raven
let i = 0;
while (i < 5) {
    print("Step: " + str(i));
    i = i + 1;
}
```

### Infinite Loop (`loop`)
Syntactic sugar for infinite loops with `break` and `continue`:
```raven
loop {
    let msg = queue.recv_timeout(50);
    if (msg == nil) {
        break;
    }
    process(msg);
}
```

### For-In Loop
Iterates across arrays and map keys:
```raven
let fruits = ["apple", "banana", "cherry"];
for (let item in fruits) {
    print("Fruit: " + item);
}
```

### Pattern Matching (`match`)
```raven
let code = 200;
let message = match (code) {
    200: "Success",
    404: "Not Found",
    500: "Internal Server Error",
    _: "Unknown status"
};
```
