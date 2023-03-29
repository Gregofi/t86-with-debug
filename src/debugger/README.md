# T86 Debugger
## Commands
The command structure is more loose than LLDB, this is because the debugger has
less features than LLDB.
General form of the commands is:
```
<noun> <verb> [argument [argument...]]
```
or
```
<verb> [argument [argument...]]
```
- `breakpoint set 5` - Sets breakpoint at line 5 and enables it.
- `breakpoint enable 1` - Enable breakpoint 1.
- `breakpoint disable 1` - Disable breakpoint 1.
- `breakpoint set 0x12` - Set breakpoint on address 0x12.
- `stepi` - Single step on instruction level.
- `step` - Single step on source level.
- `step-over` - Single step over on source level.
- `step-out` - Step out on source level.
- `continue` - Continue the process execution.
