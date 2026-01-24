# LUDA

![LUDA Banner](LUDA.png)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![IDA Version](https://img.shields.io/badge/IDA-9.X-blue)
![Lua Version](https://img.shields.io/badge/Lua-5.5-2C2D72?logo=lua)
![Status](https://img.shields.io/badge/Status-In_Development-orange)

High-performance Lua scripting for IDA Pro's SDK. Direct SDK bindings, minimal overhead, built for x86_64 binary analysis.

> **Note:** LUDA is under active development and not yet production ready.

---

## Overview

LUDA provides direct IDA SDK access from Lua, enabling rapid prototyping and automation of reverse engineering workflows. Lua's minimal overhead and clean syntax make it ideal for performance-critical analysis pipelines.

**Target Platform:** x86_64 only. Other architectures are not supported.

---

## Features

- **Direct SDK Bindings** — Call native IDA SDK functions without wrappers or proxies
- **Low Overhead** — Lua's lightweight runtime keeps complex analysis responsive  
- **Clean API** — Intuitive bindings designed for reverse engineering workflows
- **Assembler Integration** — Convert assembly to bytes via Keystone engine
- **Extensible** — Minimal dependencies, easy to integrate into custom tooling

---

## Installation

1. Download the latest release or build from source
2. Place `LUDA.dll` in your IDA plugins folder: `<IDA_DIR>/plugins/`
3. Restart IDA Pro

### Building from Source
```bash
git clone https://github.com/stolevchristian/LUDA.git
cd LUDA
```

#### Dependencies

**Keystone Engine** — Required for assembly support.

Download a prebuilt library from [keystone-engine.org](https://www.keystone-engine.org/download/) or build from source:
```bash
git clone https://github.com/keystone-engine/keystone.git
cd keystone && mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ..
cmake --build . --config Release
```

Place `keystone.lib` in the `keystone/` folder.

#### Build

Open in CLion or build via CMake:
```bash
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

---

## Usage

### Read Memory
```lua
local address = 0xDEADBEEF
local bytes = memory.read(address, 5)

for i, v in ipairs(bytes) do
    print(string.format("0x%02X", v))
end
```

### Write Memory
```lua
local address = 0xDEADBEEF
memory.write(address, {0xCC, 0xCC})
```

### Disassemble
```lua
local func_addr = 0xDEADBEEF
local disasm = hexrays.disassemble(func_addr)

print(string.format("Function at 0x%X has %d instructions", func_addr, #disasm))
```

### Assemble
```lua
local bytes = assemble("mov rax, 0xF")
-- bytes = { 0x48, 0xC7, 0xC0, 0x0F, 0x00, 0x00, 0x00 }
```

---

## Changelog

### v0.2.0 — *January 2025*
- Ported build system from Visual Studio to CLion/CMake
- Added Keystone assembler integration (`assemble()` function)
- Upgraded from Lua 5.1 to Lua 5.5 for proper 64-bit support
- Improved memory read/write API stability

### v0.1.0 — *Initial Release*
- Core IDA SDK bindings
- Memory read/write operations
- Hex-Rays disassembly support
- Socket-based communication protocol for external UI

---

## Roadmap

- [ ] Pattern scanning API
- [ ] Function signature matching
- [ ] Struct/type creation bindings
- [ ] Documentation site

---

## License

MIT License. See [LICENSE](LICENSE) for details.

---

<div align="center">

[GitHub](https://github.com/stolevchristian/LUDA)

</div>
