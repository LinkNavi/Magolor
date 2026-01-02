# test-multifile-project

A Magolor project

## Project Structure

```
├── src/
│   ├── main.mg          # Entry point
│   └── modules/         # Your modules
│       └── utils.mg     # Example module
├── target/              # Build output
└── project.toml         # Project configuration
```

## Building

```bash
gear build
```

## Running

```bash
gear run
```

## Module System

Create modules in `src/` or subdirectories:

```magolor
// src/modules/math.mg
pub fn add(a: int, b: int) -> int {
    return a + b;
}
```

Import and use in main.mg:

```magolor
using modules.math;

fn main() {
    let result = add(5, 3);
}
```
