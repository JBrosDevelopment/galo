# Galo

Simple programming language that includes a interpreter, transpiler, and compiler.

## Building the Project

Adjsut the `project.json` to the desired configuration, then run:

```sh
python .\MakefileGenerator.py
make run
```

## Example

**Functions:**

```
fun add(left int, right int) int
    return left + right
end

let result int = add(5, 10)
print("5 + 10 = ", result, " math!\n")

# can not use print(add(5, 10)) because functions do not support inline calls

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

# p.x and p.y are not valid syntax
# scoping is used by space instead of dot notation
```

**Control Flow:**

```
let condition bool = true
if true == condition and not false
    print("This is true!\n")
elif condition == false or false
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
my_color set_red(my_color, 255)
```

## Requirements
- Python
- Make
- A C compiler (e.g., GCC, Clang, MSVC)

## License
This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.