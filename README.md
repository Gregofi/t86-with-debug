# The Tinyverse monorepo
![build and test](https://github.com/gregofi/thesis-monorepo/actions/workflows/build-action.yaml/badge.svg)

A repository which houses the T86 virtual machine and the debugger. Although the debugger isn't
strictly tied to the T86 architecture, it is the only one supported as of now.

## Build
To build the T86 CLI and Debugger CLI:
```
mkdir t86/build
cd t86/build
cmake ..
make -j
```
The debugger is then available at `dbg-cli/dbg-cli`, while the t86-cli is at `t86-cli/t86-cli`.
