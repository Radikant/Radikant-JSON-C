# Radikant-JSON-C

fastest

**Radikant-JSON-C** is a simple, lightweight, and dependency-free JSON parser written in C.  
It’s designed to be easy to integrate into C projects that need to handle JSON data without the overhead of larger libraries.  
The parser builds a tree structure in memory, allowing for easy traversal and data access.

## 📦 Get

Clone the repository including submodules:
```bash
git clone --recurse-submodules https://github.com/Radikant/Radikant-JSON-C
```

---

## 🚀 Features

- **Dependency-Free:** Written in standard **C11** with no external library requirements.  
- **Tree-Based Structure:** Parses JSON into an easy-to-navigate tree of `rjson_value` nodes.  
- **Supports Core JSON Types:** Handles **strings**, **numbers**, **booleans**, **nulls**, **arrays**, and **objects**.  
- **Simple API:** A small and straightforward set of functions for parsing, accessing, and cleaning up data.  
- **CMake Build System:** Comes with a clean `CMakeLists.txt` for easy compilation.

---

## 🛠️ How to Build

The project uses **CMake** to generate build files for your platform.

```bash
# Clone the repository
git clone <your-repository-url>
cd Radikant-JSON-C

# Create a build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .
