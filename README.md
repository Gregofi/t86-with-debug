# The Tinyverse monorepo
![build and test](https://github.com/gregofi/thesis-monorepo/actions/workflows/build-action.yaml/badge.svg)

A repository which houses the T86 Virtual Machine and the debugger. Although the debugger isn't
strictly tied to the T86 architecture, it is the only one supported as of now.

## Build
To build the T86 CLI and Debugger CLI:
```
mkdir src/build
cd src/build
cmake ..
make -j
# optionally
sudo make install
```
The debugger is then available at `dbg-cli/dbg-cli` (or just `dbg-cli` if it
was installed), while the t86-cli is at `t86-cli/t86-cli`.

## Credits
The vast majority of the work on the T86 Virtual machine and the design of the architecture
was made by Ivo Strejc as part of his [thesis](http://hdl.handle.net/10467/94644). We just
added the debugging interface to it.

Also, many of the complicated tests for the CLI and debugger were created thanks to Martin
Prokopic TinyC to T86 compiler, which is also a part of a thesis (not yet published).
