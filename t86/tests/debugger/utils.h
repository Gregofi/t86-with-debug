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
