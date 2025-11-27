# Galo

Simple programming language that includes a interpreter, transpiler, and compiler.

## Building the Project

Adjsut the `project.json` to the desired configuration, then run:

```sh
python .\MakefileGenerator.py
make run
```

## Example

```
fun add(left int, right int) int
    return left + right
end

let result int = add(5, 10)
print("5 + 10 = ", result, " math!\n")

# output:
# 5 + 10 = 15 math!
```

## Requirements
- Python
- Make
- A C compiler (e.g., GCC, Clang, MSVC)

## License
This project is licensed under the MIT License. See the LICENSE file for details.

## Contributing
Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.