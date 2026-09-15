# Simple AI Agent

Free C++ chat agent. No API key. No signup. No download. No pay.

Uses public free endpoints (OVH + LLM7) with Qwen and DeepSeek-style models.

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

## Models

- Qwen3-32B / Qwen3.6-27B / Qwen3-Coder (OVH)
- DeepSeek-R1-Distill (OVH)
- Llama-3.3-70B (OVH)
- gpt-oss-20b / Mistral-Nemo (LLM7)

Rate limits exist (about 2 requests per minute on OVH anonymous). Switch model if one is busy.

## MSI

Build EXE first. Then with WiX:

```
candle installer/Product.wxs -out build/Product.wixobj
light build/Product.wixobj -out build/SimpleAIAgent.msi
```
