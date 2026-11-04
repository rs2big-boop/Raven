# Functions & Closures in Raven

Functions in Raven are first-class values that can be assigned to variables, passed as arguments, returned from other functions, and stored in collections.

---

## Function Declarations

Declare named functions using the `fn` keyword:

```raven
fn add(a, b) {
    return a + b;
}

print(add(10, 25)); // 35
```

---

## Default Parameters

Raven supports optional parameters with default fallback values:

```raven
fn greet(name = "World", punctuation = "!") {
    return "Hello, " + name + punctuation;
}

print(greet());              // "Hello, World!"
print(greet("Alice"));       // "Hello, Alice!"
print(greet("Bob", " :)"));  // "Hello, Bob :)"
```

---

## Anonymous Functions & Closures

Functions capture their enclosing lexical environment:

```raven
fn create_multiplier(factor) {
    return fn(val) {
        return val * factor;
    };
}

let double = create_multiplier(2);
let triple = create_multiplier(3);

print(double(5)); // 10
print(triple(5)); // 15
```

---

## Higher-Order Functions

Raven's runtime includes built-in higher-order functions:

```raven
let numbers = [1, 2, 3, 4, 5, 6];

// map
let squared = map(numbers, x => x * x);

// filter
let evens = filter(numbers, fn(x) { return x % 2 == 0; });

// reduce
let total = reduce(numbers, fn(acc, x) { return acc + x; }, 0);
```

---

## Pipeline Operator (`|>`)

Compose function calls cleanly from left to right:

```raven
fn add_one(x) { return x + 1; }
fn square(x) { return x * x; }

let result = 4 |> add_one |> square; // (4 + 1)^2 = 25
print(result);
```
