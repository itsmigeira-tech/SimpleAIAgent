# Simple AI Agent

C++ chat app that uses your local Ollama models.

## Build EXE

Need: Visual Studio + CMake + Ollama running.

```
git clone https://github.com/itsmigeira-tech/SimpleAIAgent.git
cd SimpleAIAgent
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Run `build/Release/SimpleAIAgent.exe`

## MSI

Build EXE first. Then:

```
candle installer/Product.wxs -out build/Product.wixobj
light build/Product.wixobj -out build/SimpleAIAgent.msi
```

Needs WiX installed.
