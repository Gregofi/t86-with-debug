# ISA

### Flags

There are these flags

* OF - Overflow
* SF - Sign, copy of highest bit of result
* ZF - Zero, set if result is zero
* CF - Carry

### Special Registers

* PC - Program counter
* FLAGS - Flags register
* SP - Stack pointer
* BP - Base pointer

### Legend
* `R{X}` indicates any register, `X` is used to distinguish multiple registers
* `i{X}` indicates integer constant, `X` is used to distinguish multiple constants
* `F{X}` indicates any float register, `X` is used to distinguis multiple float registers
* `f{X}` indicates float constant, `X` is used to distinguish multiple constants
* `[{X}]` indicates access to memory, `X` indicates the address

## Instructions details

### General

Instruction | Operands | Description | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|
MOV | `R1`, `R2` | `R1 = R2`
| | `R1`, `i` | `R1 = i`
| | `R1`, `[i]` | `R1 = [1]`
| | `R1`, `[R2]` | `R1 = [R2]`
| | `R1`, `[R2 + i]` | `R1 = [R2 + i]`
| | `R1`, `[R2 * i]` | `R1 = [R2 * i]`
| | `R1`, `[R2 + R3]` | `R1 = [R2 + R3]`
| | `R1`, `[R2 + R3 * i]` | `R1 = [R2 + R3 * i]`
| | `R1`, `[R2 + i + R3]` | `R1 = [R2 + i + R3]`
| | `R1`, `[R2 + i1 + R3 * i2]` | `R1 = [R2 + i1 + R3 * i2]`
| | `R1`, `F1` | `R1 = F1` (bit copy, use `NRW` to safely convert float to int)
| | `F1`, `f` | `F1 = f`
| | `F1`, `F2` | `F1 = F2`
| | `F1`, `R1` | `F1 = R1` (bit copy, use `EXT` to safely convert int to float)
| | `F1`, `[i]` | `F1 = [i]` (bit copy)
| | `F1`, `[R1]` | `F1 = [R1]` (bit copy) 
| | `[i]`, `R1` | `[i] = R1`
| | `[i]`, `F1` | `[i] = F1` (bit copy)
| | `[i1]`, `i2` | `[i1] = i2`
| | `[R1]`, `R2` | `[R1] = R2`
| | `[R1]`, `F1` | `[R1] = F1` (bit copy)
| | `[R1]`, `i` | `[R1] = i`
| | `[R1 + i]`, `R2` | `[R1 + i] = R2`
| | `[R1 + i]`, `F1` | `[R1 + i] = F1` (bit copy)
| | `[R1 + i1]`, `i2` | `[R1 + i1] = i2`
| | `[R1 * i]`, `R2` | `[R1 * i] = R2`
| | `[R1 * i]`, `F1` | `[R1 * i] = F1` (bit copy)
| | `[R1 * i1]`, `i2` | `[R1 * i1] = i2`
| | `[R1 + R2]`, `i` | `[R1 + R2] = i`
| | `[R1 + R2]`, `R3` | `[R1 + R2] = R3`
| | `[R1 + R2]`, `F1` | `[R1 + R2] = F1` (bit copy)
| | `[R1 + R2 * i1]`, `i2` | `[R1 + R2 * i1] = i2`
| | `[R1 + R2 * i]`, `R3` | `[R1 + R2 * i] = R3`
| | `[R1 + R2 * i]`, `F1` | `[R1 + R2 * i] = F1` (bit copy)
| | `[R1 + i1 + R2]`, `i2` | `[R1 + i1 + R2] = i2`
| | `[R1 + i + R2]`, `R3` | `[R1 + i + R2] = R3`
| | `[R1 + i + R2]`, `F1` | `[R1 + i + R2] = F1` (bit copy)
| | `[R1 + i1 + R2 * i2]`, `R3` | `[R1 + i1 + R2 * i2] = R3`
| | `[R1 + i1 + R2 * i2]`, `F1` | `[R1 + i1 + R2 * i2] = F1` (bit copy)
| | `[R1 + i1 + R2 * i2]`, `i3` | `[R1 + i1 + R2 * i2] = i3`

NOP | | Do nothing

### Arithmetics

Instruction | Operands | Description | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|
ADD | `R1`, `R2` | `R1 += R2` | |
| | `R1`, `i` | `R1 += i` | |
| | `R1`, `R2 + i` | `R1 += R2 + i` | |
| | `R1`, `[i]` | `R1 += [i]` | |
| | `R1`, `[R2 + i]` | `R1 += [R2 + i]` | |
SUB | `R1`, `R2` | `R1 -= R2` | |
| | `R1`, `i` | `R1 -= i` | |
| | `R1`, `R2 + i` | `R1 -= R2 + i` | |
| | `R1`, `[i]` | `R1 -= [i]` | |
| | `R1`, `[R2 + i]` | `R1 -= [R2 + i]` | |
INC | `R1` | `R1++`
DEC | `R1` | `R1--`
NEG | `R1` | `R1 = -R1`
MUL | `R1`, `R2` | `R1 *= R2`
| | `R1`, `i` | `R1 *= i`
| | `R1`, `R2 + i` | `R1 *= R2 + i`
| | `R1`, `[i]` | `R1 *= [i]`
| | `R1`, `[R2 + i]` | `R1 *= [R2 + i]`
DIV | `R1`, `R2` | `R1 /= R2`
| | `R1`, `i` | `R1 /= i`
| | `R1`, `R2 + i` | `R1 /= R2 + i`
| | `R1`, `[i]` | `R1 /= [i]`
| | `R1`, `[R2 + i]` | `R1 /= [R2 + i]`
IMUL | `R1`, `R2` | `R1 *= R2`, signed
| | `R1`, `i` | `R1 *= i`, signed
| | `R1`, `R2 + i` | `R1 *= R2 + i`, signed
| | `R1`, `[i]` | `R1 *= [i]`, signed
| | `R1`, `[R2 + i]` | `R1 *= [R2 + i]`, signed
IDIV | `R1`, `R2` | `R1 /= R2`, signed
| | `R1`, `i` | `R1 /= i`, signed
| | `R1`, `R2 + i` | `R1 /= R2 + i`, signed
| | `R1`, `[i]` | `R1 /= [i]`, signed
| | `R1`, `[R2 + i]` | `R1 /= [R2 + i]`, signed

### Float arithmetics
Instruction | Operands | Description | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|
FADD | `F1`, `F2` | `F1 += F2`
|    | `F1`, `f` | `F1 += f`
FSUB | `F1`, `F2` | `F1 -= F2`
| | `F1`, `f` | `F1 -= f`
FMUL | `F1`, `F2` | `F1 *= F2`
|    | `F1`, `f`  | `F1 *= f`
FDIV | `F1`, `F1` | `F1 /= F2`
|    | `F1`, `f`  | `F1 /= f`

### Bit operations

Instruction | Operands | Description | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|
AND | `R1`, `R2` | `R1 &= R2`
| | `R1`, `i` | `R1 &= i`
| | `R1`, `R2 + i` | `R1 &= R2 + i`
| | `R1`, `[i]` | `R1 &= [i]`
| | `R1`, `[R2 + i]` | `R1 &= [R2 + i]`
OR | `R1`, `R2` | `R1 \|= R2`
| | `R1`, `i` | `R1 \|= i`
| | `R1`, `R2 + i` | `R1 \|= R2 + i`
| | `R1`, `[i]` | `R1 \|= [i]`
| | `R1`, `[R2 + i]` | `R1 \|= [R2 + i]`
XOR | `R1`, `R2` | `R1 ^= R2`
| | `R1`, `i` | `R1 ^= i`
| | `R1`, `R2 + i` | `R1 ^= R2 + i`
| | `R1`, `[i]` | `R1 ^= [i]`
| | `R1`, `[R2 + i]` | `R1 ^= [R2 + i]`
NOT | `R1` | `R1 = ~ R1`
LSH | `R1`, `R2` | `R1 <<= R2`
| | `R1`, `i` | `R1 <<= i`
| | `R1`, `R2 + i` | `R1 <<= R2 + i`
| | `R1`, `[i]` | `R1 <<= [i]`
| | `R1`, `[R2 + i]` | `R1 <<= [R2 + i]`
RSH | `R1`, `R2` | `R1 >>= R2`
| | `R1`, `i` | `R1 >>= i`
| | `R1`, `R2 + i` | `R1 >>= R2 + i`
| | `R1`, `[i]` | `R1 >>= [i]`
| | `R1`, `[R2 + i]` | `R1 >>= [R2 + i]`

### Compare

Instruction | Operands | Description | Detail | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|------|
CMP | `R1`, `R2` | Compare `R1` with `R2` | Sets flags the same way `SUB` would
| | `R1`, `i` | Compare `R1` with `i`
| | `R1`, `[i]` | Compare `R1` with `[i]`
| | `R1`, `[R2]` | Compare `R1` with `[R2]`
| | `R1`, `[R2 + i]` | Compare `R1` with `[R2 + i]`
FCMP | `F1`, `F2` | Compare `F1` with `F2` | Sets flags the same way `FSUB` would
| | `F1`, `f` | Compare `F1` with `f`

### Jump

Instruction | Operands | Description | Detailed condition | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|------|
JMP | `R1` | Jump to value of `R1`
| | `i` | Jump to `i`
LOOP | `R1`, `R2`| Jump to value of `R2` if `R1` != 0 and decrement `R1` | |
| | `R1`, `i`| Jump to `i` if `R1` != 0 and decrement `R1` | |
JZ | `R1` | Jump to value of `R1` if zero | `ZF == 1` |
| | `i` | Jump to `i` if zero | |
| | `[i]` | Jump to value of `[i]` if zero | |
| | `[R1]` | Jump to value of `[R1]` if zero | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if zero | |
JNZ | `R1` | Jump to value of `R1` if not zero | `ZF == 0` |
| | `i` | Jump to `i` if not zero | |
| | `[i]` | Jump to value of `[i]` if not zero | |
| | `[R1]` | Jump to value of `[R1]` if not zero | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if not zero| |
JE | `R1` | Jump to value of `R1` if equal | `ZF == 1`
| | `i` | Jump to `i` if equal| |
| | `[i]` | Jump to value of `[i]` if equal| |
| | `[R1]` | Jump to value of `[R1]` if equal| |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if equal| |
JNE | `R1` | Jump to value of `R1` if not equal | `ZF != 1`
| | `i` | Jump to `i` if not equal| |
| | `[i]` | Jump to value of `[i]` if not equal| |
| | `[R1]` | Jump to value of `[R1]` if not equal| |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if not equal| |
JG | `R1` | Jump to value of `R1` if greater | `ZF == 0 && SF == OF`
| | `i` | Jump to `i` if greater | |
| | `[i]` | Jump to value of `[i]` if greater | |
| | `[R1]` | Jump to value of `[R1]` if greater | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if greater| |
JGE | `R1` | Jump to value of `R1` if greater or equal | `SF == OF`
| | `i` | Jump to `i` if greater or equal | |
| | `[i]` | Jump to value of `[i]` if greater or equal | |
| | `[R1]` | Jump to value of `[R1]` if greater or equal | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if greater or equal| |
JL | `R1` | Jump to value of `R1` if less | `SF != OF`
| | `i` | Jump to `i` if less| |
| | `[i]` | Jump to value of `[i]` if less | |
| | `[R1]` | Jump to value of `[R1]` if less | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if less| |
JLE | `R1` | Jump to value of `R1` if less or equal | `ZF == 1 && SF != OF`
| | `i` | Jump to `i` if less or equal| |
| | `[i]` | Jump to value of `[i]` if less or equal | |
| | `[R1]` | Jump to value of `[R1]` if less or equal | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if less or equal| |
JA | `R1` | Jump to value of `R1` if above | `CF == 0 && ZF == 0`
| | `i` | Jump to `i` if above| |
| | `[i]` | Jump to value of `[i]` if above | |
| | `[R1]` | Jump to value of `[R1]` if above | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if above| |
JAE | `R1` | Jump to value of `R1` if above or equal | `CF == 0`
| | `i` | Jump to `i` if above or equal | |
| | `[i]` | Jump to value of `[i]` if above or equal | |
| | `[R1]` | Jump to value of `[R1]` if above or equal | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if above or equal | |
JB | `R1` | Jump to value of `R1` if below | `CF == 1`
| | `i` | Jump to `i` if below ||
| | `[i]` | Jump to value of `[i]` if below ||
| | `[R1]` | Jump to value of `[R1]` if below ||
| | `[R1 + i]` | Jump to value of `[R1 + i]` if below ||
JBE | `R1` | Jump to value of `R1` if below or equal | `CF == 1 \|\| ZF == 1`
| | `i` | Jump to `i` if below or equal ||
| | `[i]` | Jump to value of `[i]` if below or equal ||
| | `[R1]` | Jump to value of `[R1]` if below or equal ||
| | `[R1 + i]` | Jump to value of `[R1 + i]` if below or equal ||
JO | `R1` | Jump to value of `R1` if overflow |`OF == 1`
| | `i` | Jump to `i` if overflow
| | `[i]` | Jump to value of `[i]` if overflow
| | `[R1]` | Jump to value of `[R1]` if overflow
| | `[R1 + i]` | Jump to value of `[R1 + i]` if overflow
JNO | `R1` | Jump to value of `R1` if not everflow | `OF == 0` |
| | `i` | Jump to `i` if not overflow
| | `[i]` | Jump to value of `[i]` if not overflow
| | `[R1]` | Jump to value of `[R1]` if not overflow
| | `[R1 + i]` | Jump to value of `[R1 + i]` if not overflow
JS | `R1` | Jump to value of `R1` if sign | `SF == 1` |
| | `i` | Jump to `i` if sign | |
| | `[i]` | Jump to value of `[i]` if sign | |
| | `[R1]` | Jump to value of `[R1]` if sign | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if sign | |
JNS | `R1` | Jump to value of `R1` if not sign | `SF == 0` |
| | `i` | Jump to `i` if not sign | |
| | `[i]` | Jump to value of `[i]` if not sign | |
| | `[R1]` | Jump to value of `[R1]` if not sign | |
| | `[R1 + i]` | Jump to value of `[R1 + i]` if not sign | |

### Call

Instruction | Operands | Description | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|
CALL | `R1` | Pushes current `PC` and jumps to value of `R1` |
| | `i` | Pushes current PC and jumps to `i` |
RET | | Jumps to top of stack and pops stack

### Stack

Instruction | Operands | Description | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|
PUSH | `R1` | Pushes `R1` to stack
| | `i` | Pushes `i` to stack
FPUSH | `F1` | Pushes `F1` to stack
|	  | `f` | Pushes `f` to stack
POP | `R1` | Stores top of stack to `R1` and pops stack
FPOP | `F1` | Stores top of stack to `F1` and pops stack

### I/O

Instruction | Operands | Description | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|
PUTCHAR | `R1` | Prints `R1` as ASCII
PUTNUM | `R1` | Prints contents of `R1` as integer followed by newline
GETCHAR | `R1` | Loads char as ASCII from input to `R1`

### Float manipulation
Instruction | Operands | Description | Length (B)| Cycle time |
----------|----------|-------------|-----------|------------|
EXT | `F1`, `R1` | extends value of `R1` to double and stores it into `F1`
NRW | `R1`, `F1` | narrows value of `F1` to int and stores it into `R1`

### Other
Instruction | Operands | Description |
----------|----------|--------|
DBG | debug function | executes debug function
BREAK | | executes handle function
HALT | | halts the CPU

## VM

__Note__: All code examples expect you to use namespace `tiny::t86`, it is not enforced on you, it is omitted for better readability.

### Configuration
You can configure your VM by adding arguments to the executed program\
To set register count, use `-registerCnt=X` - default is 10 (you can use large number of registers to begin with).\
To set float register count, use `-floatRegisterCnt=X` - default is 5.\
To set number of ALUs, use `-aluCnt=X` - default is 1.\
To set number of reservation station entries, use `-reservationStationEntriesCnt=X` - default is 2.\
To set RAM size, use `-ram=X` - default is 1024 64bit values (so total size will be 8*X bytes).\
To set RAM gate count, use `-ramGates=X` - default is 4.

__Note__: You can check config from like in this example:
```c++
Cpu::Config::instance().registerCnt();
```

### Creating program
```
ProgramBuilder pb;
pb.add(MOV{Reg(0), 42});
pb.add(MOV{Mem(Reg(0) + 27), 23});
pb.add(HALT{});

return pb.program();
```

__Note__: Do not forget to add `HALT`, otherwise your program will run forever executing only `NOP`s.

### Running program example
```c++
StatsLogger::instance().reset();
Cpu cpu;

cpu.start(std::move(program));
while (!cpu.halted()) {
    cpu.tick();
}
StatsLogger::instance().processBasicStats(std::cerr);
```
__Note__: For more detailed stats you can add:
```c++
StatsLogger::instance().processDetailedStats(std::cerr);
```

### Patching labels
```c++
ProgramBuilder pb;
Label jumpToBody = pb.add(JMP{Label::empty()});
...
Label body = pb.add(MOV{Reg(0), 0});
...
pb.patch(jumpToBody, body);
```

### Adding data
```c++
DataLabel str = pb.addData("Hello world\n");
pb.add(MOV{Reg(0), str});
```
Data will be stored starting on address 0 and further.
__Note__: This string storing is very wasteful, you can create your own packed data (I am sure you will be rewarded extra points).

### Accessing CPU registers
```c++
cpu.getRegister(Reg(0));
```

### Accessing CPU memory
```c++
cpu.getMemory(Mem(0))
```
__Note__: Memory is addresable by 8bytes (64bit values)

### Debug and handle function
Debug and handle functions have to have this function signature
```c++
void fn(Cpu&)
```
You can hook one handle function (executed on `BREAK`) by
```c++
cpu.connectBreakHandler(&fn);
```
or using c++11 lambdas. \
Debug example:
```c++
pb.add(DBG{&fn});
```
__Note__ that DBG will be added only if your `ProgramBuilder` was not given `true` argument indicating release environment.

### Other notes
There are some example is `tests/targets/tiny86/programs.cpp`.\
If you encounter any bug, please don't hesitate to report it.