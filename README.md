# Simple AI Agent

Free C++ chat agent. No API key. No signup. No download. No pay.

## Build (easy)

1. Download the ZIP from GitHub and extract it.
2. Open the folder that contains CMakeLists.txt and build.cmd.
3. Double-click build.cmd
4. Allow Administrator if asked. Wait for installs and build.
5. EXE opens in Explorer: build\Release\SimpleAIAgent.exe

## Manual build

Need Visual Studio Build Tools + CMake.

```
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Rate limits

- LLM7 models: about 10 requests per minute
- Kilo free pool: about 200 requests per hour
- OVH models: about 2 requests per minute

Higher rate models are listed first. Auto-fallback on rate limit.

## MSI

Build EXE first. Then with WiX:

```
candle installer/Product.wxs -out build/Product.wixobj
light build/Product.wixobj -out build/SimpleAIAgent.msi
```
