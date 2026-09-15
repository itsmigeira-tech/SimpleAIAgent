# Simple AI Agent

Lightweight C++ AI agent. Uses local models already installed with Ollama.

Native Win32 UI. No ImGui. No external UI libraries.

## Requirements

- Windows 10 or later
- Visual Studio 2019 or newer (with C++ desktop development)
- CMake 3.16+
- Ollama installed and running (https://ollama.com)
- At least one model pulled (example: ollama pull llama3.2)

## Build the EXE

Open Developer Command Prompt for VS.

```
git clone https://github.com/itsmigeira-tech/SimpleAIAgent.git
cd SimpleAIAgent
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

EXE is at build/Release/SimpleAIAgent.exe

Run it. Keep Ollama running in the background.

## MSI Installer

Use WiX Toolset (https://wixtoolset.org).

1. Install WiX.
2. From the project root run:

```
candle installer/Product.wxs -out build/Product.wixobj
light build/Product.wixobj -out build/SimpleAIAgent.msi
```

Or open the solution in Visual Studio and add a WiX project.

The installer places SimpleAIAgent.exe under Program Files and creates a Start Menu shortcut.

## How it works

- On start the agent runs "ollama list" and fills the model dropdown.
- You type a prompt and click Send.
- It posts to http://localhost:11434/api/generate
- Response appears in the chat window.

No models are downloaded by the agent. It only uses what you already installed.

## Notes

- If Ollama is not in PATH the default model name is used.
- Firewall must allow localhost:11434.
- For other local servers (llama.cpp) change the host and path in main.cpp.
