# Source level debugging
This document introduces the format necessary for the source level debugger to work properly.

## Line to address mapping
To allow breakpoints be set on a source line, the debugger needs a mapping between assembly addresses
and the source lines of the program.

Consider the following program
```c
int main() {
    int a = 5;
    int *b = &a;
    *b = 4;
    return a;
}
```
which compiles into
```
.text
0 CALL 2
1 HALT
2 PUSH BP
3 MOV BP,SP
4 NOP
5 SUB SP,1
6 SUB SP,1
7 MOV R0,5
8 MOV [BP + -2],R0
9 LEA R1,[BP + -2]
10 MOV [BP + -1],R1
11 MOV R2,4
12 MOV R3,[BP + -1]
13 MOV [R3],R2
14 MOV R4,[BP + -2]
15 MOV R5,R4
16 JMP 17
17 NOP
18 ADD SP,2
19 POP BP
20 RET
```

The mapping would be included in another section, namely `.debug_line` and would look like this
```
.debug_line
0: 7
1: 7
2: 9
3: 11
4: 14
5: 18
```

Notice that the first point of the function is considered at the point after prologue and the last point
of the function is before epilogue. As a compiler designer, you can make the decision yourself if breakpoint
on a function should break before or after prologue/epilogue.

The lines not need be continuous. For example, you can have mapping like this
```
2: 0
3: 0
5: 1
```
The debugger will simply report an error if you try to set breakpoint at source line `0`, `1` or `4`.
They also needn't be ordered.

## Metadata
The debugger requires various metadata to properly print identifiers. For example the code
```c
struct coord {
    int x;
    int y;
};

int main() {
    double d = 3.5;
    
    int x = 3;
    int y = 1;

    struct coord s;
    s.x = x;
    s.y = y;
}
```
When using `variable get v`, you would expect that `3.5` is printed. The debugger needs to know that
the value must be interpreted as double. Here is an example of a metadata with a source program:

```
.debug_metadata
Type:
 - Name: double
 - Size: 1
 - Type: floating-point

Type:
 - Name: int
 - Size: 1
 - Type: signed

StructuredType
 - Name: coord
 - Members:
   - 0:int
   - 1:int

CompilationUnit:
 - Path: /path/to/structs.tc
 - StartingPoint: main
 Function:
  - Name: main
  - BeginAddr: 0
  - BeginSource: 
  - EndAddr: 8
  Variable:
    - Name: d
    - Type: double
  Variable:
    - Name: x
    - Type: int
  Variable: 5
    - Name: y
    - Type: int

.debug_loc
5: 0
6: 0
7: 2
8: 2
9: 3
10: 4
11: 4
12: 4
13: 6
14: 8

.text
0 movsd   F0, 3.5
1 movsd   [BP-2], F0
2 mov     [BP-3], 3
3 mov     [BP-4], 1
4 mov     R0, [BP-3]
5 mov     [BP-8], R0
6 mov     R0, [BP-4]
7 mov     [BP-7], R0
8 HALT
```

With this additional info, the breakpoints on functions will work as well.
The beginning source location of an function will be used for that fact.
The addresses are used to inform you where you currently are, in case
you did instruction level single step into the function prologue.
In case you won't provide the beginning source line of a function
or if the `.debug_line` section is missing, then the beginning address
is used for functions breakpoint, but be aware that at this point the
prologue wasn't run.

You don't have to include every debugging information. For example, you
can only use this minimum:
```
.debug_metadata
CompilationUnit:
 - Path: /path/to/exe.tc

.debug_line
...
```

So that we can map the location in the program and tell you where the
breakpoint exactly happened.
