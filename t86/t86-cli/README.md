# T86VM CLI
Command line interface for the T86 virtual machine. Parses an assembly of T86 and runs it on the virtual machine.

## Assembly grammar

Heavily TBD

address         = "0x", number
instruction     = [address] ins_name { operand }
code            = ".text" { instruction }
register        = R, number

s = code

### Sections
The whole file is separated into few sections
- text - The instructions to be executed
- data - storage which size does not change at runtime (global/static vars)
Each section begins with ".section_name"

### Text section
Begins with ".text"

Example of text section:
```
.text
0x0 MOV R0, 1
0x1 ADD R0, 2
0x2 ADD R0, R0
0x3 SUB R0, 6
0x4 XOR R1, R1   ; Zero out R1
; R0 == R1?
0x5 CMP R0, R1
```

Each line begins with an address. This is NOT mandatory and the address is actually ignored. It is hovewer encouraged to include it because the file is alot more readable. Next is the name of the instruction itself. Those names are consistent with the names provided in T86 README. Then the operand follows. They may be arbitrary number of them, depending on the instruction used. `;` begins an comment. Those may be on the end of the line or on an otherwise empty line.

Operands are the same as in T86 documentation. Beware when using substraction, for example `0x0 MOV R0, R1 - 1` **HAS** to be written as `0x0 MOV R0, R1 + -1`.

