# Simple AI Agent

Free C++ chat agent. No API key. No signup. No download. No pay.

## Build EXE

Need Visual Studio + CMake.

```
git clone https://github.com/itsmigeira-tech/SimpleAIAgent.git
cd SimpleAIAgent
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Run `build/Release/SimpleAIAgent.exe`

## Rate limits (improved)

- LLM7 models: about 10 requests per minute
- Kilo free pool: about 200 requests per hour
- OVH models: about 2 requests per minute

Higher rate models are listed first. If one hits a limit the agent auto-tries the next.

## MSI

Build EXE first. Then with WiX:

```
candle installer/Product.wxs -out build/Product.wixobj
light build/Product.wixobj -out build/SimpleAIAgent.msi
```
