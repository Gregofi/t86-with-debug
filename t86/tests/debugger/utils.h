#pragma once
#include <istream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "common/threads_messenger.h"
#include "debugger/Process.h"
#include "debugger/Source/Parser.h"
#include "t86/os.h"
#include "debugger/Source/Source.h"

class MockedProcess: public Process {
public:
    MockedProcess(std::vector<std::string> text, std::vector<int64_t> data,
                  std::map<std::string, int64_t> regs): text(std::move(text)),
                                data(std::move(data)), regs(std::move(regs)) { }

    void WriteText(uint64_t address,
                   const std::vector<std::string> &text) override {
        for (size_t i = 0; i < data.size(); ++i) {
            this->text[i + address] = text[i];
        }
    }
    std::vector<std::string> ReadText(uint64_t address,
                                      size_t amount) override {
        std::vector<std::string> result;
        for (size_t i = address; i < address + amount; ++i) {
            result.push_back(text[i]);
        }
        return result;
    }

    void WriteMemory(uint64_t address, const std::vector<int64_t>& data) override {
        for (size_t i = 0; i < data.size(); ++i) {
            this->data[i + address] = data[i];
        }
    }

    std::vector<int64_t> ReadMemory(uint64_t address, size_t amount) override {
        std::vector<int64_t> result;
        for (size_t i = address; i < address + amount; ++i) {
            result.push_back(data[i]);
        }
        return result;
    }

    StopReason GetReason() override {
        NOT_IMPLEMENTED;
    }

    /// Performs singlestep and returns true.
    /// Should throws runtime_error if architecture
    /// does not support singlestepping.
    void Singlestep() override {
        NOT_IMPLEMENTED;
    }

    std::map<std::string, int64_t> FetchRegisters() override {
        return regs;
    }
    std::map<std::string, double> FetchFloatRegisters() override {
        NOT_IMPLEMENTED;
    }
    void SetRegisters(const std::map<std::string, int64_t>& regs) override {
        this->regs = regs;
    }
    void SetFloatRegisters(const std::map<std::string, double>& regs) override {
        NOT_IMPLEMENTED;
    }
    std::map<std::string, uint64_t> FetchDebugRegisters() override {
        NOT_IMPLEMENTED;
    }
    virtual void SetDebugRegisters(const std::map<std::string, uint64_t>& regs) override {
        NOT_IMPLEMENTED;
    }
    void ResumeExecution() override {
        NOT_IMPLEMENTED;
    }
    size_t TextSize() override {
        NOT_IMPLEMENTED;
    }
    void Wait() override {
        NOT_IMPLEMENTED;
    }
    /// Cause the process to end, the class should not be used
    /// after this function is called.
    void Terminate() override {
        NOT_IMPLEMENTED;
    }
protected:
    std::vector<std::string> text;
    std::vector<int64_t> data;
    std::map<std::string, int64_t> regs;
};

inline void RunCPU(std::unique_ptr<ThreadMessenger> messenger,
            const std::string &program, size_t register_cnt = 4, size_t float_cnt = 0) {
    std::istringstream iss{program};
    Parser parser(iss);
    auto p = parser.Parse();
    
    tiny::t86::OS os(register_cnt, float_cnt);
    os.SetDebuggerComms(std::move(messenger));
    os.Run(std::move(p));
}

class NativeTest: public ::testing::Test {
protected:
    void Run(const char* elf, size_t gp_regs, size_t fl_regs) {
        auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
        auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
        t_os = std::thread(RunCPU, std::move(tm1), elf, gp_regs, fl_regs);
        auto t86 = std::make_unique<T86Process>(std::move(tm2), gp_regs, fl_regs);
        native = std::make_optional<Native>(std::move(t86));
    }

    void TearDown() override {
        native->Terminate();
        t_os.join();
    }

    std::optional<Native> native;
    std::thread t_os;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
};

class T86ProcessTest: public ::testing::Test {
protected:
    void Run(const char* elf, size_t gp_regs, size_t fl_regs) {
        auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
        auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
        t_os = std::thread(RunCPU, std::move(tm1), elf, gp_regs, fl_regs);
        t86.emplace(std::move(tm2), gp_regs, fl_regs);
    }

    void TearDown() override {
        t86->Terminate();
        t_os.join();
    }
    std::optional<T86Process> t86;
    std::thread t_os;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
};

class NativeSourceTest: public ::testing::Test {
protected:
    void Run(const char* elf) {
        const size_t REG_COUNT = 6;
        auto tm1 = std::make_unique<ThreadMessenger>(q1, q2);
        auto tm2 = std::make_unique<ThreadMessenger>(q2, q1);
        t_os = std::thread(RunCPU, std::move(tm1), elf, REG_COUNT, 0);
        auto t86 = std::make_unique<T86Process>(std::move(tm2), REG_COUNT, 0);
        native = std::make_optional<Native>(std::move(t86));

        std::istringstream iss(elf);
        dbg::Parser p(iss);
        auto debug_info = p.Parse();
        source.RegisterLineMapping(std::move(*debug_info.line_mapping));
        source.RegisterDebuggingInformation(std::move(*debug_info.top_die));
        source.RegisterSourceFile(std::move(*debug_info.source_code));
    }

    void TearDown() override {
        native->Terminate();
        t_os.join();
    }

    Source source;
    std::optional<Native> native;
    std::thread t_os;
    ThreadQueue<std::string> q1;
    ThreadQueue<std::string> q2;
};

class LinkedListNativeSourceTest: public NativeSourceTest {
protected:
    void SetUp() override {
    auto program = R"(
.text
0       CALL 2
1       HALT
# MAIN:
2       PUSH    BP
3       MOV     BP, SP
4       SUB     SP, 8
5       MOV     [BP + -4], 5
6       LEA     R1, [BP + -6]
7       MOV     [BP + -3], R1
8       MOV     R1, [BP + -3]
90      MOV     [R1], 10
10      MOV     R1, [BP + -3]
11      LEA     R2, [BP + -8]
12      MOV     [R1 + 1], R2
13      MOV     R1, [BP + -3]
14      MOV     R1, [R1 + 1]
15      MOV     [R1], 15
16      MOV     R1, [BP + -3]
17      MOV     R1, [R1 + 1]
18      MOV     [R1 + 1], 0
19      LEA     R1, [BP + -4]
20      MOV     [BP + -1], R1
21      JMP     28
# .L3:
22      MOV     R1, [BP + -1]
23      MOV     R1, [R1]
24      PUTNUM  R1
25      MOV     R1, [BP + -1]
26      MOV     R1, [R1 + 1]
27      MOV     [BP + -1], R1
# .L2:
28      MOV     R0, [BP + -1]
29      CMP     R0, 0
30      JNE     22
31      MOV     R0, 0
32      ADD SP, 8
33      POP BP
34      RET

.debug_line
8: 2
9: 5
10: 5
11: 5
12: 5
13: 6
14: 8
15: 10
16: 13
17: 16
18: 19
19: 19
20: 28
21: 22
22: 25
24: 32

.debug_info
DIE_compilation_unit: {
DIE_structured_type: {
    ATTR_size: 2,
    ATTR_id: 1,
    ATTR_name: "struct list",
    ATTR_members: {
        0: {0: v},
        1: {2: next},
    },
},
DIE_pointer_type: {
    ATTR_size: 1,
    ATTR_type: 1,
    ATTR_id: 2,
},
DIE_primitive_type: {
    ATTR_name: int,
    ATTR_size: 1,
    ATTR_id: 0,
},
DIE_function: {
    ATTR_name: main,
    ATTR_begin_addr: 2,
    ATTR_end_addr: 35,
    DIE_scope: {
        ATTR_begin_addr: 2,
        ATTR_end_addr: 35,
        DIE_variable: {
            ATTR_name: l1,
            ATTR_type: 1,
            ATTR_location: `BASE_REG_OFFSET -4`,
        },
        DIE_variable: {
            ATTR_name: l2,
            ATTR_type: 1,
            ATTR_location: `BASE_REG_OFFSET -6`,
        },
        DIE_variable: {
            ATTR_name: it,
            ATTR_type: 2,
            ATTR_location: `BASE_REG_OFFSET -1`,
        }
    },
}
}

.debug_source
#include <stdlib.h>
#include <stdio.h>

struct list {
    int v;
    struct list* next;
};

int main() {
    struct list l1;
    struct list l2;
    struct list l3;
    l1.v = 5;
    l1.next = &l2;
    l1.next->v = 10;
    l1.next->next = &l3;
    l1.next->next->v = 15;
    l1.next->next->next = NULL;

    struct list* it = &l1;
    while (it != NULL) {
        printf("%d\n", it->v);
        it = it->next;
    }
}
)";
        Run(program);
    }
};

class SwapNativeSourceTest: public NativeSourceTest {
    void SetUp() override {
    auto program =
R"(.text
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
    ATTR_type: 0,
    ATTR_id: 1,
    ATTR_size: 1,
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
            ATTR_location: ``,
        },
        DIE_variable: {
            ATTR_name: y,
            ATTR_type: 1,
            ATTR_location: ``,
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
})";
        Run(program); 
    }
};

class BinarySearchNativeSourceTest: public NativeSourceTest {
protected:
    void SetUp() override {
        auto program = R"(
.text
#     
# ======= FUNCTION entry START =======
# entry$entry:
   0  PUSH BP
   1  MOV BP, SP
#     entry prologue end
#     58 -> main$entry
   2  CALL 58
   3  HALT
# ======= FUNCTION entry END =======
#     
#     
# ======= FUNCTION binary_search START =======
# binary_search$entry:
   4  PUSH BP
   5  MOV BP, SP
   6  SUB SP, 8
   7  MOV [BP + -7], R2
   8  MOV [BP + -8], R3
#     binary_search prologue end
#     [BP + 2] -> arg #0
   9  MOV R0, [BP + 2]
#     [BP + 3] -> arg #1
  10  MOV R2, [BP + 3]
#     [BP + 4] -> arg #2
  11  MOV R3, [BP + 4]
#     [BP + -1] -> %arg_arr
  12  MOV [BP + -1], R0
#     [BP + -2] -> %arg_length
  13  MOV [BP + -2], R2
#     [BP + -3] -> %arg_n
  14  MOV [BP + -3], R3
#     [BP + -4] -> %local_lo
  15  MOV [BP + -4], 0
#     [BP + -2] -> %arg_length
  16  MOV R0, [BP + -2]
#     [BP + -5] -> %local_hi
  17  MOV [BP + -5], R0
# binary_search$whileCond:
#     [BP + -4] -> %local_lo
  18  MOV R0, [BP + -4]
#     [BP + -5] -> %local_hi
  19  MOV R2, [BP + -5]
  20  CMP R0, R2
#     52 -> binary_search$whileCont
  21  JGE 52
# binary_search$whileBody:
#     [BP + -4] -> %local_lo
  22  MOV R0, [BP + -4]
#     [BP + -5] -> %local_hi
  23  MOV R2, [BP + -5]
  24  ADD R0, R2
  25  IDIV R0, 2
#     [BP + -6] -> %local_ptr
  26  MOV [BP + -6], R0
#     [BP + -1] -> %arg_arr
  27  MOV R0, [BP + -1]
#     [BP + -6] -> %local_ptr
  28  MOV R2, [BP + -6]
  29  MOV R0, [R0 + R2 * 1]
#     [BP + -3] -> %arg_n
  30  MOV R2, [BP + -3]
  31  CMP R0, R2
#     37 -> binary_search$ifFalse
  32  JGE 37
# binary_search$ifTrue:
#     [BP + -6] -> %local_ptr
  33  MOV R0, [BP + -6]
  34  ADD R0, 1
#     [BP + -4] -> %local_lo
  35  MOV [BP + -4], R0
# binary_search$ifCont:
#     18 -> binary_search$whileCond
  36  JMP 18
# binary_search$ifFalse:
#     [BP + -1] -> %arg_arr
  37  MOV R0, [BP + -1]
#     [BP + -6] -> %local_ptr
  38  MOV R2, [BP + -6]
  39  MOV R0, [R0 + R2 * 1]
#     [BP + -3] -> %arg_n
  40  MOV R2, [BP + -3]
  41  CMP R0, R2
#     46 -> binary_search$ifFalse2
  42  JLE 46
# binary_search$ifTrue2:
#     [BP + -6] -> %local_ptr
  43  MOV R0, [BP + -6]
#     [BP + -5] -> %local_hi
  44  MOV [BP + -5], R0
# binary_search$ifCont2:
#     36 -> binary_search$ifCont
  45  JMP 36
# binary_search$ifFalse2:
  46  MOV R0, 1
#     binary_search epilogue start
  47  MOV R2, [BP + -7]
  48  MOV R3, [BP + -8]
  49  MOV SP, BP
  50  POP BP
  51  RET
# binary_search$whileCont:
  52  MOV R0, 0
#     binary_search epilogue start
  53  MOV R2, [BP + -7]
  54  MOV R3, [BP + -8]
  55  MOV SP, BP
  56  POP BP
  57  RET
# ======= FUNCTION binary_search END =======
#     
#     
# ======= FUNCTION main START =======
# main$entry:
  58  PUSH BP
  59  MOV BP, SP
  60  SUB SP, 13
  61  MOV [BP + -13], R1
  62  MOV [BP + -12], R3
#     main prologue end
#     [BP + -10] -> %local_arr
  63  LEA R1, [BP + -10]
#     [BP + -11] -> %local_i
  64  MOV [BP + -11], 0
# main$forCond:
#     [BP + -11] -> %local_i
  65  MOV R0, [BP + -11]
  66  CMP R0, 10
#     75 -> main$forCont
  67  JGE 75
# main$forBody:
#     [BP + -11] -> %local_i
  68  MOV R0, [BP + -11]
#     [BP + -11] -> %local_i
  69  MOV R3, [BP + -11]
  70  MOV [R1 + R0 * 1], R3
# main$forInc:
#     [BP + -11] -> %local_i
  71  MOV R0, [BP + -11]
  72  ADD R0, 1
#     [BP + -11] -> %local_i
  73  MOV [BP + -11], R0
#     65 -> main$forCond
  74  JMP 65
# main$forCont:
  75  MOV R3, 10
  76  MOV R0, 4
  77  PUSH R0
  78  PUSH R3
  79  PUSH R1
#     4 -> binary_search$entry
  80  CALL 4
  81  ADD SP, 3
  82  MOV R0, 10
  83  MOV R3, 5
  84  PUSH R3
  85  PUSH R0
  86  PUSH R1
#     4 -> binary_search$entry
  87  CALL 4
  88  ADD SP, 3
  89  MOV R0, 10
  90  MOV R3, 10
  91  PUSH R3
  92  PUSH R0
  93  PUSH R1
#     4 -> binary_search$entry
  94  CALL 4
  95  ADD SP, 3
  96  MOV R0, 0
#     main epilogue start
  97  MOV R1, [BP + -13]
  98  MOV R3, [BP + -12]
  99  MOV SP, BP
 100  POP BP
 101  RET
# ======= FUNCTION main END =======
#     

.debug_line
0: 4
1: 15
2: 16
3: 18
4: 22
5: 27
6: 33
7: 37
8: 43
9: 46
10: 46
13: 52
14: 53
16: 58
17: 63
18: 64
19: 68
20: 71
21: 75
22: 75
23: 82
24: 89
25: 96
26: 96
27: 97

.debug_info
DIE_compilation_unit: {
DIE_primitive_type: {
    ATTR_name: int,
    ATTR_size: 1,
    ATTR_id: 0,
},
DIE_pointer_type: {
    ATTR_id: 1,
    ATTR_type: 0,
    ATTR_size: 1,
},
DIE_array_type: {
    ATTR_id: 2,
    ATTR_type: 0,
    ATTR_size: 10,
},
DIE_function: {
    ATTR_name: binary_search,
    ATTR_begin_addr: 4,
    ATTR_end_addr: 58,
    DIE_scope: {
        ATTR_begin_addr: 4,
        ATTR_end_addr: 58,
        DIE_variable: {
            ATTR_name: arr,
            ATTR_type: 1,
            ATTR_location: [BASE_REG_OFFSET -1],
        },
        DIE_variable: {
            ATTR_name: length,
            ATTR_type: 0,
            ATTR_location: [BASE_REG_OFFSET -2],
        },
        DIE_variable: {
            ATTR_name: n,
            ATTR_type: 0,
            ATTR_location: [BASE_REG_OFFSET -3],
        },
        DIE_variable: {
            ATTR_name: lo,
            ATTR_type: 0,
            ATTR_location: [BASE_REG_OFFSET -4],
        },
        DIE_variable: {
            ATTR_name: hi,
            ATTR_type: 0,
            ATTR_location: [BASE_REG_OFFSET -5],
        },
        DIE_scope: {
            ATTR_begin_addr: 22,
            ATTR_end_addr: 46,
            DIE_variable: {
                ATTR_name: ptr,
                ATTR_type: 0,
                ATTR_location: [BASE_REG_OFFSET -6],
            },
        },
    },
},
DIE_function: {
    ATTR_name: main,
    ATTR_begin_addr: 58,
    ATTR_end_addr: 102,
    DIE_scope: {
        ATTR_begin_addr: 58,
        ATTR_end_addr: 102,
        DIE_variable: {
            ATTR_name: arr,
            ATTR_type: 2,
            ATTR_location: [BASE_REG_OFFSET -10],
        },
        DIE_scope: {
            ATTR_begin_addr: 64,
            ATTR_end_addr: 75,
            DIE_variable: {
                ATTR_name: i,
                ATTR_type: 0,
                ATTR_location: [BASE_REG_OFFSET -11],
            }
        }
    }
}
}

.debug_source
int binary_search(int* arr, int length, int n) {
    int lo = 0;
    int hi = length;
    while (lo < hi) {
        int ptr = (lo + hi) / 2;
        if (arr[ptr] < n) {
            lo = ptr + 1;
        } else if (arr[ptr] > n) {
            hi = ptr;
        } else {
            return 1;
        }
    }
    return 0;
}

int main() {
    int arr[10];
    for (int i = 0; i < 10; ++i) {
        arr[i] = i;
    }

    binary_search(arr, 10, 4);
    binary_search(arr, 10, 5);
    binary_search(arr, 10, 10);

    return 0;
}
)";
    Run(program);
    }
};
