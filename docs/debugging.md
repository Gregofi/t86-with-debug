# Debuggging
The debugger has two layers. First is a _native_ (or assembly level) debugging.
This allows you to debug on the instruction level, without any information
about the original source language. You do not require any debugging
information for this. Only register count and memory size needs to be provided
if your program uses more than the default.

The debugger has an extensible help built into it. Use the command `help` to
see general help. To get subcommands of a command, use the `help <command>`,
for example `help breakpoint`. This text serves as an insight of how some
things work so that you're not suprised by some of the behavior.

The debugger uses prefixes for the command. For example you can write `h b` instead
of `help breakpoint`. Do note that if same commands share the same prefix (like `frame`
and `finish`), the one that is displayed first in the `help` command will take
precedence.

The debugger is also trying to show as many information as possible when you
debug. For instance, when you step or break, it shows the assembly (or lines,
if available) where the break happened.

Many of the commands occur twice, one is meant for source debugging, one
for native debugging. The native commands are prefixed with `i`.
For example the `step` "executes" one line of the source code, whereas
`istep` executes one instruction.

Unlike GDB or LLDB, you **need** to run the program before setting breakpoints,
but when the program is run, it immediately stops before executing the
first instruction.

## GDB to DBG map

A map of some of the commands that we share with GDB, showing possible ways
of writing them. This list is of course not exhaustive, nor are the examples
showing all possible ways of how you might write the command.

<table>
    <thead>
        <tr>
            <th>GDB</th>
            <th>T86-DBG</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td colspan=2>Setting a breakpoint at line 5</td>
        </tr>
        <tr>
            <td><samp> b 5</samp></td>
            <td><samp> b s 5</samp></td>
        </tr>
        <tr>
            <td colspan=2>run the process and immediately stop</td>
        </tr>
        <tr>
            <td><samp> start </samp></td>
            <td><samp> run </samp></td>
        </tr>
        <tr>
            <td colspan=2>Do a source level step</td>
        </tr>
        <tr>
            <td><samp> s </samp></td>
            <td><samp> s </samp></td>
        </tr>
        <tr>
            <td><samp> step </samp></td>
            <td><samp> step </samp></td>
        </tr>
        <tr>
            <td colspan=2>Do an instruction level step</td>
        </tr>
        <tr>
            <td rowspan=2><samp>stepi</samp></td>
            <td><samp>istep</samp></td>
        </tr>
        <tr>
            <td><samp>is</samp></td>
        </tr>
        <tr>
            <td colspan=2>Step out of current function</td>
        </tr>
        <tr>
            <td><samp>finish</samp></td>
            <td><samp>finish</samp></td>
        <tr>
            <td colspan=2>Set breakpoint on main function</td>
        </tr>
        <tr>
            <td><samp>break main</samp></td>
            <td><samp>b s main</samp></td>
        </tr>
        <tr>
            <td colspan=2>List all breakpoints</td>
        </tr>
        <tr>
            <td><samp>info break</samp></td>
            <td><samp>br list</samp></td>
        </tr>
        <tr>
            <td colspan=2>Disable a breakpoint</td>
        </tr>
        <tr>
            <td><samp>disable &lt;id&gt;</samp></td>
            <td><samp>br dis &lt;addr&gt;</td>
        </tr>
        <tr>
            <td colspan=2>Show the content of variable "a"</td>
        </tr>
        <tr>
            <td rowspan=2><samp>p a</samp></td>
            <td><samp>p a</td>
        </tr>
        <tr>
            <td><samp>expr a</td>
        </tr>
        <tr>
            <td colspan=2>Disable a breakpoint</td>
        </tr>
        <tr>
            <td><samp>disable &lt;id&gt;</samp></td>
            <td><samp>br dis &lt;addr&gt;</td>
        </tr>
    </tbody>
</table>

## Commands
### Run
Runs the program, stopping before the first instruction is executed.
Optionaly specify the number of registers and size of the RAM memory.

### Attach
Attach to a running VM, to debug the program it is running.

### Breakpoints
You can set, remove, enable and disable breakpoint on any address or line.
The commands for lines are in form `breakpoint set <line>`, to set it
at address, prefix the subcommand with `i`, ie. `breakpoint iset 5` sets
breakpoint on instruction at address `5`.

Do note that the breakpoints are `indexed` by the address they are set on
(unlike GDB or LLDB). So if you want to remove breakpoint use the `breakpoint
remove <addr>`, where the `addr` is the address they were set on.

### Stepping
The native part offers three ways of stepping.
- `istep` = Execute one instruction.
- `inext` = Execute one instruction, if it is a call then continue execution
  and stop after the call instruction (effectively stepping over the call).
- `finish` = Continue execution until the `RET` of the current function is executed,
  effectively stepping out of the function.

Do note that if any other event happens while stepping (like breakpoint hit or HALT),
then the program is stopped at that point.

For step out and over to work correctly in recursive functions it is needed to
properly backup base pointer (`PUSH BP; MOV BP, SP`). It uses it as a heuristic
to know that the expected frame has finished.

The source level debugging offers:
- `step` = Continue execution until another source line is encountered.
- `next` = Same as step, but step over `CALL` instructions.

Note that the steps are continuing execution **until another source line is encountered.**
So if no source line is available, these commands will continue until the `HALT` happens
or breakpoint is encountered.

### Watchpoints
Sets a watchpoint on a memory address that breaks on memory writes to that address.

### Disassemble
Prints the underlying assembly that is being debugged. Shows the current position
in the program and displays breakpoints.

### Assemble
Rewrite the underlying assembly. This is only able to rewrite what already exists,
it is not able to add new instructions or erase them (although for that you
may just rewrite the instruction with a `NOP`).

It works through an interactive mode. Use the `assemble interactive 2`.
Now you can write instructions which will be written to the program
starting at address two. When you want to stop just leave the line
empty. Ie.

```
> assemble int 3
3: > NOP
4: > MOV [2], 5
5: > NOP
6: >              # empty line
```

### Registers
Fetch and set register values.
The normal and float registers are separated, use the `get, set` for normal registers
and `fget, fset` for float registers.

### Memory
Read and write into debuggee memory.
The commands have two forms, for example the read command is either
- `get <addr> <amount>` = Read an `<amount>` of memory cells beginning at address `<address>`
- `gets <addr> <amount>` = The same as `get`, but interprets the result as an ASCII string.
Same with the `set` command

### Source
Display the underlying debugged source code.
Requires `.debug_source` section in the executable.

### Frame
Displays function in which is the execution currently located.
Also displays active variables and their values.
Requires all debugging information.

### Expression
A very powerful command, is able to display and set values of variables,
but can also evaluate a `C` like expressions. There is a `print` alias
to this command.
An examples:
- `expression a` - Prints the value of variable `a`
- `e *a` - Dereferences `a` and prints the value.
- `e *a = 5` - Assings value `5` to the location that `a` points to.

Arrays and structs are also supported, see the `t86/tests/debugger/source_test.cpp` tests
to get a glimpse of all the possible things.
