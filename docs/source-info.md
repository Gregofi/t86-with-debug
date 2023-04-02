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
which could compile into (using a very primitive compiler)
```
.text
0 CALL 2
1 HALT
2 PUSH BP
3 MOV BP, SP
4 NOP
5 SUB SP, 1
6 SUB SP, 1
7 MOV R0, 5
8 MOV [BP - 2], R0
9 LEA R1, [BP - 2]
10 MOV [BP - 1], R1
11 MOV R2, 4
12 MOV R3, [BP - 1]
13 MOV [R3], R2
14 MOV R4, [BP - 2]
15 MOV R5, R4
16 JMP 17
17 NOP
18 ADD SP, 2
19 POP BP
20 RET
```

The mapping would be included in section `.debug_line` and could look like this
```
.debug_line
0: 2
1: 7
2: 9
3: 11
4: 14
5: 20
```

Notice that the first point of the function is considered at the beginning of
prologue and the last point of the function is after epilogue. As a compiler
designer, you can make the decision yourself if a breakpoint on a function
should break before or after prologue/epilogue. The mapping begins from zero!
The line `int main() {` is considered as line `0`. The debugger however works
with them as you would expect, so a breakpoint on line `1` sets it on line `int
main() {`.

The lines not need be continuous. For example, you can have mapping like this
```
2: 0
3: 0
5: 1
```
The debugger will simply report an error if you try to set breakpoint at source line `0`, `1` or `4`.
They also needn't be ordered.

## Source file
To display the source where breakpoint happened (or just to view it in general), you must provide
the source file.
```
other sections...

.debug_source
int main() {
    int a = 5;
    int *b = &a;
    *b = 4;
    return a;
}
```
Do note that this must be the **last** section in the file.
Adding this section allows the debugger to report where
various incidents happened.

## Metadata
The debugger requires various metadata to properly print identifiers. For example the code
```c
void swap(int* x, int* y) {
    int tmp = *x;
    *x = *y;
    *y = tmp;
}

int main() {
    int a = 3;
    int b = 6;
    swap(&a, &b);
    print(a);
    print(b);
}
```
When using `expression *x`, you would expect that the `x` is interpreted as a
pointer and is properly dereferenced. For this, the debugger needs to know the
type of the variable, and its location, so that it might get its value. The
line below shows how a source information about this program should look like:

```
.text
0       CALL    7            # main
1       HALT
# swap(int*, int*):
2       MOV     R0, [R2]
3       MOV     R1, [R3]
4       MOV     [R2], R1
5       MOV     [R3], R0
6       RET
# main:
7       MOV     [SP + -2], 3   # a
8       MOV     [SP + -3], 6   # b
9       LEA     R2, [SP + -2]
10      LEA     R3, [SP + -3]
11      CALL    2             # swap(int*, int*)
12      MOV     R0, [SP + -2]
13      PUTNUM  R0
14      MOV     R1, [SP + -3]
15      PUTNUM  R1
16      XOR     R0, R0        # return 0
17      RET

.debug_line
0: 2
1: 2
2: 3
3: 5
4: 6
6: 7
7: 7
8: 8
9: 9
10: 12
11: 14
12: 16

.debug_info
DIE_compilation_unit: {
DIE_primitive_type: {
    ATTR_name: int,
    ATTR_id: 0,
    ATTR_size: 1,
},
DIE_pointer_type: {
    ATTR_type: 1,
    ATTR_size: 1,
    ATTR_id: 1,
},
DIE_function: {
    ATTR_name: main,
    ATTR_begin_addr: 7,
    ATTR_end_addr: 18,
    DIE_scope: {
        ATTR_begin_addr: 7,
        ATTR_end_addr: 18,
        DIE_variable: {
            ATTR_name: a,
            ATTR_type: 0,
            ATTR_location: [PUSH SP; PUSH -2; ADD],
        },
        DIE_variable: {
            ATTR_name: b,
            ATTR_type: 0,
            ATTR_location: [PUSH SP; PUSH -3; ADD],
        }
    }
},
DIE_function: {
    ATTR_name: swap,
    ATTR_begin_addr: 2,
    ATTR_end_addr: 7,
    DIE_scope: {
        ATTR_begin_addr: 2,
        ATTR_end_addr: 7,
        DIE_variable: {
            ATTR_name: x,
            ATTR_type: 1,
            ATTR_location: [PUSH R2],
        },
        DIE_variable: {
            ATTR_name: y,
            ATTR_type: 1,
            ATTR_location: [PUSH R3],
        },
        DIE_variable: {
            ATTR_name: tmp,
            ATTR_type: 0,
            ATTR_location: `PUSH R0`,
        }
    }
}
}

.debug_source
void swap(int* x, int* y) {
    int tmp = *x;
    *x = *y;
    *y = tmp;
}

int main() {
    int a = 3;
    int b = 6;
    swap(&a, &b);
    print(a);
    print(b);
}
```
It is under the `.debug_info` section, the structure is similar to JSON. There
are two main types of keys. One is prefixed with `DIE_`, representing debugging
information entry. Those entries have various attributes, beginning with
`ATTR_` prefix. You don't have to generate all debugging information at once,
but rather can do so incrementally. You can find more examples under
`src/dbg-cli/tests/sources`.

There are following types of DIEs:
### `DIE_compilation_unit`
This must be a top-level DIE. It is the only one that cannot be a child of
another DIE. It is a mandatory if you include this section.

Allowed children: Everything but `DIE_scope`.

### `DIE_function`
Represents a function.

Attributes:
- `ATTR_name`: Name of the function.
- `ATTR_begin_addr`: Address of the first instruction of the function.
- `ATTR_end_addr`: Address of the next instruction after the last one in the function (last one + 1).

Allowed children:
- `DIE_scope`: The body of the function.

### `DIE_scope`
Represents a scope.

Attributes:
- `ATTR_begin_addr`: Address of the first instruction belonging to the scope.
- `ATTR_end_addr`: Address of the next instruction after the last one belonging to the scope (last one + 1).

Allowed children:
- `DIE_scope`: A subscope.
- `DIE_variable`: A Variable active in the scope.

The scopes serve mainly as a indicator for the lifetime of a variable. If there is a scope without any
variable declaration you don't have to generate it, because it doesn't add any information.

### `DIE_variable`
Represents a variable declaration

Attributes:
- `ATTR_name`: Name of the variable.
- `ATTR_location`: Location of the variable, is a location expression, see the
  _Location expressions_ section.
- `ATTR_type`: ID of the DIE that represents this variable type.

### `DIE_primitive_type`
Represents either an int, float or char type.

Attributes:
- `ATTR_name`: Name of the primitive type, either `int`, `float` or `char`.
- `ATTR_size`: Size of the primitive type, for T86 it **must** be 1.
- `ATTR_id`: ID of the type, must be unique for every DIE that specifies it.
  Other dies may refer to this one using this ID.

### `DIE_pointer_type`
Represents a pointer type.

Attributes:
- `ATTR_type`: ID of the DIE that represents a type that this pointer points to.
- `ATTR_size`: Size of the pointer type, for T86 it **must** be 1.
- `ATTR_id`: ID of the type, must be unique for every DIE that specifies it.
  Other dies may refer to this one using this ID.

### `DIE_structured_type`
Represents a structured type (a `struct` in C).

Example of a structured type information:
```
DIE_structured_type: {
    ATTR_size: 2,
    ATTR_id: 1,
    ATTR_name: "struct list",
    ATTR_members: {
        0: {0: v},
        1: {2: next},
    },
},
```
Corresponds to this type:
```
struct list {
    int v;
    struct list* next;
};
```

Attributes:
- `ATTR_name`: Name of the structured type.
- `ATTR_size`: Size of the struct type. In T86 it'll probably be the same as the sum of
  sizes of its members. This is mainly for architectures that aling the objects.
- `ATTR_id`: ID of the type, must be unique for every DIE that specifies it.
  Other dies may refer to this one using this ID.
- `ATTR_members`: A list of the members of the struct. In form of `{ <structured-member> }`,
  where `<structured-member>` is a `<offset>: {<type>: <name>}`. The `<offset>` is offset from
  the beginning location of the struct. The `<type>` is an ID of the type that the member has,
  and `<name>` is the name of the type.

### `DIE_array_type`
Represents a static array.

Attributes:
- `ATTR_size`: Number of elements of the array. This is **not** the total size of the array!
  It may occupy more memory if the elements are not of size 1.
- `ATTR_id`: ID of the type, must be unique for every DIE that specifies it.
  Other dies may refer to this one using this ID.
- `ATTR_type`: ID of the type of the elements in the array.

## Location expressions
The location of the variables are described by a simple virtual machine program.
The virtual machine is a stack based one. The value at the top of the stack after computation
is done is considered to be the location of the variable.

Instructions of the virtual machine:
- `PUSH <op>` - Pushes the operand onto the stack.
- `ADD` - Pops two values from the stack and adds them together.
- `BASE_REG_OFFSET x` - Takes the value in `BP` register and adds `x` to it. Pushes that result onto the stack.
- `DEREFERENCE` - Pops a value `x` from the stack. Pushes value in memory cell `x` onto the stack.

The operand is either a register (`R0`, `IP`, `BP`...) or an integer.
The register is considered a register until some other operation happens.
For example the `PUSH R0` pushes register R0 onto the stack. If we however
add `PUSH 1` and `ADD`, the result would be the value in register `R0` added to
`1`, which is an offset, not a register. If the value is in register, just use
`PUSH R0`, if it is at `[R0]`, use `PUSH 0; PUSH R0; ADD`.

The program itself is written either in backticks `` `BASE_REG_OFFSET -2` `` or
in square brackets `[PUSH 0; PUSH R0; ADD]`. The backticks allow only zero or
one instruction, the brackets allow arbitrary amount. If some variable has an
unknown location, you can either provide an empty (`[]`) program or just leave
the information out altogether.
