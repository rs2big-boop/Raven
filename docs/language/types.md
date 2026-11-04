# Raven Type System & Introspection

Raven is dynamically typed with first-class runtime type inspection and structured record types.

---

## Primitive Types

| Type | Syntax / Examples | Description |
|---|---|---|
| `Nil` | `nil` | Represents the absence of a value. |
| `Number` | `42`, `3.14159`, `-10` | 64-bit IEEE-754 double precision floating point. |
| `String` | `"hello"`, `"""multi-line"""` | UTF-8 encoded string. |
| `Bool` | `true`, `false` | Boolean truth values. |
| `Array` | `[1, 2, "three"]` | Ordered, dynamically sized list. |
| `Map` | `{ key: "value" }`, `{ data, next }` | Hash map. Unquoted keys and JS-style shorthand are preferred. |
| `Function` | `fn(x) { return x; }` | Callable closures and functions. |

---

## Type Introspection with `is`

Raven features the `is` operator for clean, expressive runtime type checking:

```raven
let val = 42;
if (val is Number) {
    print("val is numeric");
}

let name = "Raven";
if (name is String) {
    print("name is text");
}

let items = [1, 2, 3];
if (items is Array) {
    print("items is a list");
}
```

The `type(v)` builtin returns the lowercase type name string:
```raven
print(type(42));        // "number"
print(type("test"));    // "string"
print(type([1, 2]));    // "array"
print(type({}));        // "map"
```

---

## Struct Declarations

Raven supports structured types using the `struct` keyword:

```raven
struct Point {
    x,
    y
}

let p = Point(10, 20);
print(p.x); // 10
print(p.y); // 20
```

Struct instances are tagged maps that can also be inspected with the `is` operator:
```raven
if (p is Point) {
    print("Valid point instance");
}
```
