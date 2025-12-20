# Galo

Simple programming language that includes a interpreter, transpiler, and compiler.

## Building the Project

This project uses a Makefile for simplicity

```sh
make run
```

For debugging using gdb, run

```sh
make run-debug
```

- `make`: builds program
- `make debug`: builds program with `-g` flags for debugging
- `make clean`: clears `bin/` folder
- `make run`: builds and runs program
- `make run-debug`: builds program with `-g` flag and runs it inside `gdb` for debugging

## Example

**Functions:**

```
fun add(left int, right int) int
    # inside parenthesis because all operations require parenthesis to define order of operations
    return (left + right) 
end

let result int = add(5, 10)
print("5 + 10 = ", result, " math!\n")

# output:
# 5 + 10 = 15 math!
```

**Structs:**

```
struct Point
    x int
    y int
end

let p Point
p x = 10
p y = 20

let X int = p x

# p.x and p.y are not valid syntax
# scoping is used by space instead of dot notation
```

**Control Flow:**

```
let condition bool = true
if condition
    print("This is true!\n")
elif ((condition == false) or false)
    print("Oh no it's false!\n")
else
    print("This is false!\n")
end

while condition
    print("Looping...\n")
    condition = false
end
```

**Functions in Structs**

```
struct Color
    r byte
    g byte
    b byte
end

fun Color init(red byte, green byte, blue byte) Color
    let c Color
    c r = red
    c g = green
    c b = blue
    return c
end

fun Color set_red(self Color, value byte) void
    self r = value
end

let my_color Color = Color init(0, 0, 0)
Color set_red(my_color, 255)
```

**Data Types**
byte, int, float, bool, string, and user-defined structs are supported.

```
let b byte = 255
let i int = 12345
let b2 byte = (b + i) # this will not raise an error because validator doesn't check bytes, overflow over 255 will rap back to 0
let f float = 3.14
let s string = f # raises an error, cannot assign float to string
```

## Requirements
- Make
- A C compiler (e.g., GCC, Clang, MSVC)
- GDB (optional, for development debugging with `make debug`)

## License
This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.