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
.debug_info
DIE_primitive_type: {
ATTR_name: double,
ATTR_size: 1,
},

DIE_primitive_type: {
ATTR_name: int,
ATTR_size: 1,
},

DIE_structured_type: {
ATTR_name: coord,
ATTR_size: 2,
ATTR_members: {
    0:int,
    1:int,
  }
},

DIE_function: {
  ATTR_name: main,
  ATTR_begin_addr: 0,
  ATTR_end_addr: 8,
  DIE_scope: {
    ATTR_begin_addr: 0,
    ATTR_end_addr: 8, 
    DIE_variable: {
      ATTR_name: d,
      ATTR_type: double,
      ATTR_location: `BASE_REG_OFFSET -2`,
    },
    DIE_variable: {
      ATTR_name: x,
      ATTR_type: int,
      ATTR_location: [PUSH BP; PUSH -3; ADD],
    }
    DIE_variable: {
      ATTR_name: y,
      ATTR_type: int,
      ATTR_location: `BASE_REG_OFFSET -2`,
    }
  }
}


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

The debugging information is JSON like. It somewhat represents
the actual program with its tree like structure.
