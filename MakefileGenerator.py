import os
import glob
import json
import platform

def get_source_files(source_dirs):
    files = []
    for d in source_dirs:
        files.extend(glob.glob(os.path.join(d, "**", "*.c"), recursive=True))
    return files

def normalize_path(path):
    return path.replace("/", "\\") if platform.system() == "Windows" else path

def generate_option_flags(options_config):
    """Convert JSON options into -D flags for GCC."""
    flags = []
    for opt in options_config:
        name = opt.get("name")
        choices = opt.get("options", [])
        value_index = opt.get("value", 0)
        if choices:
            flags.append(f"-D{name}={value_index}")

        index = 0
        for c in choices:
            flags.append(f"-D{c}={index}")
            index += 1
    return " ".join(flags)

def generate_makefile(config):
    name = config.get("name", "project")
    out_dir = config.get("out-dir", "bin")
    compiler = config.get("compiler", "gcc")
    base_flags = " ".join(config.get("flags", []))
    include_dirs = " ".join(f"-I{inc}" for inc in config.get("include", []))
    libraries = " ".join(config.get("library", []))
    sources = get_source_files(config.get("source", ["src"]))

    option_flags = generate_option_flags(config.get("options", []))
    all_flags = f"{base_flags} {include_dirs} {option_flags}".strip()

    objects = []
    object_rules = ""
    for src in sources:
        rel_path = os.path.relpath(src, start=os.path.commonpath(config.get("source", ["src"])))
        obj_path = os.path.join(out_dir, os.path.splitext(rel_path)[0] + ".o")
        objects.append(normalize_path(obj_path))

        obj_dir = os.path.dirname(obj_path)
        obj_rule = f"""
{normalize_path(obj_path)}: {normalize_path(src)}
\t@mkdir -p {normalize_path(obj_dir)}
\t{compiler} {all_flags} -c {normalize_path(src)} -o {normalize_path(obj_path)}
"""
        object_rules += obj_rule

    objects_str = " ".join(objects)

    mkdir_cmd = "@mkdir -p $(OUT_DIR)"
    clean_cmd = "rm -rf $(OUT_DIR)/*.o $(TARGET)"

    makefile_content = f"""
CC = {compiler}
CFLAGS = {all_flags}
LDFLAGS = {libraries}
OUT_DIR = {normalize_path(out_dir)}
TARGET = $(OUT_DIR)/{name}

SOURCES = {' '.join(normalize_path(s) for s in sources)}
OBJECTS = {objects_str}

all: $(OUT_DIR) $(TARGET) remove_objects

remove_objects:
	rm -f $(OBJECTS)

$(OUT_DIR):
\t{mkdir_cmd}

$(TARGET): $(OBJECTS)
\t$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

{object_rules}

clean:
\t{clean_cmd}
"""

    if config.get("make-run", {}).get("enable", False):
        args = " ".join(config["make-run"].get("args", []))
        makefile_content += f"""
run: $(TARGET) remove_objects
\t./$(TARGET) {args}
"""

    return makefile_content

# Load config
with open("project.json") as f:
    config = json.load(f)

makefile_text = generate_makefile(config)

with open("Makefile", "w") as f:
    f.write(makefile_text)

print("Makefile generated successfully!")
