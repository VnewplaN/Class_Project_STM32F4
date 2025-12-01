## VS Code setup for Ctrl+Click (Go to Definition)

This workspace contains built-in settings to enable Ctrl+Click (Go to Definition) in VS Code for C/C++ using either the **C/C++ (ms-vscode.cpptools)** extension or **clangd**.

Steps to enable:
1. Install the C/C++ extension by Microsoft (ms-vscode.cpptools) or clangd.
2. Generate the build (CMake) so `compile_commands.json` exists at `build/Debug/compile_commands.json`. Run: `cmake --build build --config Debug`.
3. VS Code should automatically pick up the `c_cpp_properties.json` and `settings.json` in `.vscode`.

Notes:
- If your `compile_commands.json` is in a different build folder, edit `.vscode/c_cpp_properties.json` `compileCommands` to point to the right file.
- If Ctrl+Click doesn't work, reload the window or restart VS Code, and ensure the extension is installed and active.
