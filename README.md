![LUDA Banner](LUDA.png)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![IDA Version](https://img.shields.io/badge/IDA-9.X-blue)
![Lua Version](https://img.shields.io/badge/Lua-5.1-2C2D72?logo=lua)
![Status](https://img.shields.io/badge/Status-Active-green)

> **High-performance Lua scripting for IDA Pro's SDK. Fast, lightweight, and expressive binary analysis automation.**

> [!IMPORTANT]  
> **LUDA** is still WIP and is not production ready. It has recently switched to using Lua 5.5 instead of Lua 5.1 due to Lua 5.1 not supporting 64-bit correctly.

---
## UI Preview
#### The user interface is not yet public but if you want to create your own you can do so by reading comm protocol in ludasocket.h and ludasocket.cpp.
![Beta Preview](https://i.imgur.com/KOU4bun.png)
---
LUDA provides **direct SDK access** from Lua, enabling rapid prototyping and automation of reverse engineering workflows in IDA Pro 9.2. Unlike Python or IDC scripting, Lua offers minimal overhead, exceptional performance, and a clean API designed specifically for security research and binary analysis.

---

## Features

- ✅ **Direct IDA SDK Bindings** — Call native SDK functions from Lua without wrappers or proxies
- ✅ **Performance Optimized** — Lua's speed makes complex analysis pipelines responsive
- ✅ **Clean API** — Intuitive bindings that feel natural for IDA scripting
- ✅ **Rapid Prototyping** — Fast iteration for security research and vulnerability analysis
- ✅ **Lightweight** — Minimal dependencies, embeddable in custom workflows

---

### Read Memory
```lua
-- Read Template
-- Replace the address with your own

local data_address = 0xDEADBEEF
local bytes = memory.read(data_address, 5) -- Read 5 bytes
for i,v in next, bytes do
  print(i, v)
end
```

### Write Memory
```lua
-- Write Template
-- Replace the address with your own

local data_address = 0xDEADBEEF
memory.write(data_address, {0xCC,0xCC})
```

### Disassemble
```lua
-- Disassemble Template
-- Replace the address with your own

local function_address = 0xDEADBEEF
local disassembly = hexrays.disassemble(function_address)
print(table.concat({
  "The function", 
  "0x" .. hex(function_address), 
  "has", #disassembly, "instructions."
}, " "))
```
---

<div align="center">

**Made with ❤️ for the game**

*Building better reverse engineering tools, one bug at a time!*

[⭐ Star this project if you find it useful!](https://github.com/stolevchristian/LUDA)

</div>
