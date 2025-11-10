![LUDA Banner](LUDA.png)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![IDA Version](https://img.shields.io/badge/IDA-9.X-blue)
![Lua Version](https://img.shields.io/badge/Lua-5.1-2C2D72?logo=lua)
![Status](https://img.shields.io/badge/Status-Active-green)

> **High-performance Lua scripting for IDA Pro's SDK. Fast, lightweight, and expressive binary analysis automation.**

> [!IMPORTANT]  
> **LUDA** is still in development and is not ready for production. Expect lack of error handling, bugs and crashes.

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

### Basic Usage
```lua
-- Simple script
local target = get_function("_verifyIntegrity")
local xrefs  = get_xrefs(target)

for idx, ref in next, xrefs do
	print("anticheat called at:", hex(ref))
end
```

---

<div align="center">

**Made with ❤️ for the game**

*Building better reverse engineering tools, ome bug at a time!*

[⭐ Star this project if you find it useful!](https://github.com/stolevchristian/LUDA)

</div>
