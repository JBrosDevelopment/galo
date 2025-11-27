
CC = gcc
CFLAGS = -Wall -Wextra -O2  -DLanguageType=2 -DCompiler=0 -DTranspiler=1 -DInterpreter=2
LDFLAGS = 
OUT_DIR = bin
TARGET = $(OUT_DIR)/galo

SOURCES = src\compiler.c src\interpreter.c src\lexer.c src\main.c src\parser.c src\transpiler.c src\validator.c
OBJECTS = bin\compiler.o bin\interpreter.o bin\lexer.o bin\main.o bin\parser.o bin\transpiler.o bin\validator.o

all: $(OUT_DIR) $(TARGET) remove_objects

remove_objects:
	rm -f $(OBJECTS)

$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)


bin\compiler.o: src\compiler.c
	@mkdir -p bin
	gcc -Wall -Wextra -O2  -DLanguageType=2 -DCompiler=0 -DTranspiler=1 -DInterpreter=2 -c src\compiler.c -o bin\compiler.o

bin\interpreter.o: src\interpreter.c
	@mkdir -p bin
	gcc -Wall -Wextra -O2  -DLanguageType=2 -DCompiler=0 -DTranspiler=1 -DInterpreter=2 -c src\interpreter.c -o bin\interpreter.o

bin\lexer.o: src\lexer.c
	@mkdir -p bin
	gcc -Wall -Wextra -O2  -DLanguageType=2 -DCompiler=0 -DTranspiler=1 -DInterpreter=2 -c src\lexer.c -o bin\lexer.o

bin\main.o: src\main.c
	@mkdir -p bin
	gcc -Wall -Wextra -O2  -DLanguageType=2 -DCompiler=0 -DTranspiler=1 -DInterpreter=2 -c src\main.c -o bin\main.o

bin\parser.o: src\parser.c
	@mkdir -p bin
	gcc -Wall -Wextra -O2  -DLanguageType=2 -DCompiler=0 -DTranspiler=1 -DInterpreter=2 -c src\parser.c -o bin\parser.o

bin\transpiler.o: src\transpiler.c
	@mkdir -p bin
	gcc -Wall -Wextra -O2  -DLanguageType=2 -DCompiler=0 -DTranspiler=1 -DInterpreter=2 -c src\transpiler.c -o bin\transpiler.o

bin\validator.o: src\validator.c
	@mkdir -p bin
	gcc -Wall -Wextra -O2  -DLanguageType=2 -DCompiler=0 -DTranspiler=1 -DInterpreter=2 -c src\validator.c -o bin\validator.o


clean:
	rm -rf $(OUT_DIR)/*.o $(TARGET)

run: $(TARGET) remove_objects
	./$(TARGET) 
