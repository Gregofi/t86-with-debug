# The Tinyverse monorepo
![build and test](https://github.com/gregofi/thesis-monorepo/actions/workflows/build-action.yaml/badge.svg)

A repository that houses all thing T86 needed for the thesis.
It has a TinyC to T86 compiler, which will probably go away soon.
The T86 folder contains an T86 library and CLI through which one
may parse and run T86 files. It also has a debugger which is strictly
not tied to the T86 architecture, but so far only supports that one.

## Build
To build the T86 CLI and Debugger CLI:
```
mkdir t86/build
cd t86/build
cmake ..
make -j
```
The debugger is then available at `dbg-cli/dbg-cli`.
