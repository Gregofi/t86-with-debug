#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sstream>
#include <variant>
#include <thread>
#include "debugger/Source/Parser.h"
#include "debugger/Source/LineMapping.h"
#include "debugger/Source/Source.h"
#include "threads_messenger.h"
#include "utils.h"

TEST(SourceLines, Parsing) {
    auto program = R"(
.debug_line
0: 3
1: 3
2: 4
3: 5
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    EXPECT_EQ(info.line_mapping->size(), 4);
    EXPECT_EQ(info.line_mapping->at(0), 3);
    EXPECT_EQ(info.line_mapping->at(1), 3);
    EXPECT_EQ(info.line_mapping->at(2), 4);
    EXPECT_EQ(info.line_mapping->at(3), 5);
}

TEST(SourceLines, ParsingNonContinuous) {
    auto program = R"(
.debug_line
0: 3

5: 3

9:

4
1: 5
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    EXPECT_EQ(info.line_mapping->size(), 4);
    EXPECT_EQ(info.line_mapping->at(0), 3);
    EXPECT_EQ(info.line_mapping->at(5), 3);
    EXPECT_EQ(info.line_mapping->at(9), 4);
    EXPECT_EQ(info.line_mapping->at(1), 5);
}

TEST(SourceLines, ParsingEmpty) {
    auto program = R"(
.debug_line
.text
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    EXPECT_EQ(info.line_mapping->size(), 0);
}

TEST(LineMapping, Basics) {
    auto program = R"(
.debug_line
0: 3
1: 3
2: 4
3: 5
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    ASSERT_TRUE(info.line_mapping);
    LineMapping lm(std::move(*info.line_mapping));
    EXPECT_EQ(lm.GetAddress(0), 3);
    EXPECT_EQ(lm.GetAddress(1), 3);
    EXPECT_EQ(lm.GetAddress(2), 4);
    EXPECT_EQ(lm.GetAddress(3), 5);

    EXPECT_THAT(lm.GetLines(3), testing::ElementsAre(0, 1));
    EXPECT_THAT(lm.GetLines(4), testing::ElementsAre(2));
    EXPECT_THAT(lm.GetLines(5), testing::ElementsAre(3));
}

TEST(Source, LineAndSourceMapping) {
    Source source;
    ASSERT_FALSE(source.AddrToLine(0));

    auto program = R"(
.debug_line
0: 3
1: 3
2: 5
3: 5
4: 7
5: 7
6: 8

.debug_source
int main(void) {
    int i = 5;

    int y = 10;

    return i + y;
}
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    ASSERT_TRUE(info.line_mapping);
    LineMapping lm(std::move(*info.line_mapping));
    source.RegisterLineMapping(std::move(lm));
    ASSERT_FALSE(source.AddrToLine(0));
    ASSERT_FALSE(source.AddrToLine(1));
    ASSERT_FALSE(source.AddrToLine(2));
    ASSERT_TRUE(source.AddrToLine(3));
    EXPECT_EQ(source.AddrToLine(3), 1);
    ASSERT_TRUE(source.AddrToLine(5));
    EXPECT_EQ(source.AddrToLine(5), 3);
    ASSERT_TRUE(source.AddrToLine(7));
    EXPECT_EQ(source.AddrToLine(7), 5);
    ASSERT_TRUE(source.AddrToLine(8));
    EXPECT_EQ(source.AddrToLine(8), 6);

    ASSERT_THAT(source.GetLines(0, 3), testing::IsEmpty());
    source.RegisterSourceFile(*info.source_code);
    auto source_program = source.GetLines(0, 10);
    ASSERT_EQ(source_program.size(), 7);
    EXPECT_EQ(source_program.at(0), "int main(void) {");
    EXPECT_EQ(source_program.at(1), "    int i = 5;");
    EXPECT_EQ(source_program.at(2), "");
    EXPECT_EQ(source_program.at(3), "    int y = 10;");
    EXPECT_EQ(source_program.at(4), "");
    EXPECT_EQ(source_program.at(5), "    return i + y;");
    EXPECT_EQ(source_program.at(6), "}");
}

TEST_F(NativeSourceTest, SourceBreakpoints) {
    const char* elf =
R"(
.text
0 CALL 2
1 HALT
2 PUSH BP
3 MOV BP, SP
4 SUB SP, 2
5 MOV [BP + -1], 5
6 MOV [BP + -2], 6
7 MOV R0, [BP + -1]
8 MOV R1, [BP + -2]
9 ADD R0, R1
10 ADD SP, 2
11 POP BP
12 RET

.debug_line
0: 2
1: 5
2: 6
3: 7
4: 11

.debug_source
int main() {
    int a = 5;
    int b = 6;
    return a + b;
}
)";
    Run(elf);

    for (size_t i = 0; i < 5; ++i) {
        ASSERT_TRUE(source.LineToAddr(i));
    }

    EXPECT_EQ(source.LineToAddr(0), 2);
    EXPECT_EQ(source.LineToAddr(1), 5);
    EXPECT_EQ(source.LineToAddr(2), 6);
    EXPECT_EQ(source.LineToAddr(3), 7);
    EXPECT_EQ(source.LineToAddr(4), 11);

    ASSERT_TRUE(std::holds_alternative<ExecutionBegin>(native->WaitForDebugEvent()));
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(*native, 0), 2);
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(*native, 1), 5);
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(*native, 2), 6);
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(*native, 3), 7);
    EXPECT_EQ(source.SetSourceSoftwareBreakpoint(*native, 4), 11);
    ASSERT_THROW({source.SetSourceSoftwareBreakpoint(*native, 5);}, DebuggerError);

    native->ContinueExecution();
    ASSERT_TRUE(std::holds_alternative<BreakpointHit>(native->WaitForDebugEvent()));
    EXPECT_EQ(native->GetIP(), 2);

    native->ContinueExecution();
    ASSERT_TRUE(std::holds_alternative<BreakpointHit>(native->WaitForDebugEvent()));
    EXPECT_EQ(native->GetIP(), 5);

    native->ContinueExecution();
    ASSERT_TRUE(std::holds_alternative<BreakpointHit>(native->WaitForDebugEvent()));
    EXPECT_EQ(native->GetIP(), 6);

    native->ContinueExecution();
    ASSERT_TRUE(std::holds_alternative<BreakpointHit>(native->WaitForDebugEvent()));
    EXPECT_EQ(native->GetIP(), 7);
    // Check BP - 1 and BP - 2
    auto bp = native->GetRegister("BP");
    EXPECT_EQ(native->ReadMemory(bp - 1, 1)[0], 5);
    EXPECT_EQ(native->ReadMemory(bp - 2, 1)[0], 6);
    native->ContinueExecution();

    ASSERT_TRUE(std::holds_alternative<BreakpointHit>(native->WaitForDebugEvent()));
    EXPECT_EQ(native->GetIP(), 11);
    EXPECT_EQ(native->GetRegister("R0"), 11);
    native->ContinueExecution();

    ASSERT_TRUE(std::holds_alternative<ExecutionEnd>(native->WaitForDebugEvent()));
}

TEST(DIEs, Parsing1) {
    auto program = 
R"(.debug_info
DIE_function: {
    ATTR_name: main,
}
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    const auto& top_die = info.top_die;
    ASSERT_TRUE(top_die);
    ASSERT_EQ(top_die->get_tag(), DIE::TAG::function);
    ASSERT_EQ(top_die->begin(), top_die->end());
    ASSERT_NE(top_die->begin_attr(), top_die->end_attr());
    auto attr = *top_die->begin_attr();
    ASSERT_TRUE(std::holds_alternative<ATTR_name>(attr));
    EXPECT_EQ(std::get<ATTR_name>(attr).n, "main");
}

TEST(DIEs, Parsing2) {
    auto program = 
R"(.debug_info
DIE_function: {
    ATTR_name: main,
    ATTR_begin_addr: 0,
    ATTR_end_addr: 10,
    DIE_variable: {
        ATTR_name: d,
    },
}
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    const auto& top_die = info.top_die;
    ASSERT_TRUE(top_die);
    ASSERT_EQ(top_die->get_tag(), DIE::TAG::function);

    auto it_attr = top_die->begin_attr();
    ASSERT_NE(it_attr, top_die->end_attr());
    ASSERT_TRUE(std::holds_alternative<ATTR_name>(*it_attr));
    EXPECT_EQ(std::get<ATTR_name>(*it_attr).n, "main");
    ++it_attr;
    ASSERT_NE(it_attr, top_die->end_attr());
    ASSERT_TRUE(std::holds_alternative<ATTR_begin_addr>(*it_attr));
    EXPECT_EQ(std::get<ATTR_begin_addr>(*it_attr).addr, 0);
    ++it_attr;
    ASSERT_NE(it_attr, top_die->end_attr());
    ASSERT_TRUE(std::holds_alternative<ATTR_end_addr>(*it_attr));
    EXPECT_EQ(std::get<ATTR_end_addr>(*it_attr).addr, 10);
    ++it_attr;
    ASSERT_EQ(it_attr, top_die->end_attr());

    auto it_die = top_die->begin();
    ASSERT_NE(it_die, top_die->end());
    ASSERT_EQ(it_die->get_tag(), DIE::TAG::variable);
    
    it_attr = it_die->begin_attr();
    ASSERT_NE(it_attr, it_die->end_attr());
    ASSERT_TRUE(std::holds_alternative<ATTR_name>(*it_attr));
    EXPECT_EQ(std::get<ATTR_name>(*it_attr).n, "d");
    ++it_attr;
    ASSERT_EQ(it_attr, it_die->end_attr());
}

TEST(LocationExpression, OneParsing) {
    auto program = 
R"(.debug_info
DIE_variable: {
    ATTR_location: `BASE_REG_OFFSET 0`,
}
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    const auto& top_die = info.top_die;
    ASSERT_TRUE(top_die);
    ASSERT_EQ(top_die->get_tag(), DIE::TAG::variable);
    auto it_attr = top_die->begin_attr();
    ASSERT_TRUE(std::holds_alternative<ATTR_location_expr>(*it_attr));
    const auto& loc_expr = std::get<ATTR_location_expr>(*it_attr).locs;
    ASSERT_EQ(loc_expr.size(), 1);
    ASSERT_TRUE(std::holds_alternative<expr::FrameBaseRegisterOffset>(loc_expr[0]));
}

TEST(LocationExpression, None) {
    auto program = 
R"(.debug_info
DIE_variable: {
    ATTR_location: ``,
}
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    const auto& top_die = info.top_die;
    ASSERT_TRUE(top_die);
    ASSERT_EQ(top_die->get_tag(), DIE::TAG::variable);
    auto it_attr = top_die->begin_attr();
    ASSERT_TRUE(std::holds_alternative<ATTR_location_expr>(*it_attr));
    const auto& loc_expr = std::get<ATTR_location_expr>(*it_attr).locs;
    ASSERT_EQ(loc_expr.size(), 0);
}

TEST(LocationExpression, Multiple) {
    auto program = 
R"(.debug_info
DIE_variable: {
    ATTR_location: [PUSH BP; PUSH -2; ADD],
}
)";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    const auto& top_die = info.top_die;
    ASSERT_TRUE(top_die);
    ASSERT_EQ(top_die->get_tag(), DIE::TAG::variable);
    auto it_attr = top_die->begin_attr();
    ASSERT_TRUE(std::holds_alternative<ATTR_location_expr>(*it_attr));
    const auto& loc_expr = std::get<ATTR_location_expr>(*it_attr).locs;
    ASSERT_EQ(loc_expr.size(), 3);
    ASSERT_TRUE(std::holds_alternative<expr::Push>(loc_expr[0]));
    auto e1 = std::get<expr::Push>(loc_expr[0]);
    ASSERT_TRUE(std::holds_alternative<expr::Register>(e1.value)); 
    EXPECT_EQ(std::get<expr::Register>(e1.value).name, "BP"); 
    ASSERT_TRUE(std::holds_alternative<expr::Push>(loc_expr[1]));
    auto e2 = std::get<expr::Push>(loc_expr[1]);
    EXPECT_EQ(std::get<expr::Offset>(e2.value).value, -2); 
    ASSERT_TRUE(std::holds_alternative<expr::Add>(loc_expr[2]));
}

TEST(DebugInfo, ParsingCombined) {
    auto program =
R"(.debug_info
DIE_compilation_unit: {
DIE_primitive_type: {
    ATTR_name: double,
    ATTR_size: 1,
    ATTR_id: 1,
},
DIE_primitive_type: {
    ATTR_name: int,
    ATTR_size: 1,
    ATTR_id: 0,
},
DIE_structured_type: {
    ATTR_name: coord,
    ATTR_size: 2,
    ATTR_members: {
        0: 0,
        1: 0,
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
      ATTR_type: 1,
      ATTR_location: `BASE_REG_OFFSET -2`,
    },
    DIE_variable: {
      ATTR_name: x,
      ATTR_type: 0,
      ATTR_location: [PUSH BP; PUSH -3; ADD],
    },
    DIE_variable: {
      ATTR_name: y,
      ATTR_type: 0,
      ATTR_location: `BASE_REG_OFFSET -4`,
    }
  }
}
})";
    std::istringstream iss(program);
    dbg::Parser p(iss);
    auto info = p.Parse();
    const auto& top_die = info.top_die;
    ASSERT_TRUE(top_die);
    ASSERT_EQ(top_die->get_tag(), DIE::TAG::compilation_unit);
    ASSERT_EQ(std::distance(top_die->begin(), top_die->end()), 4);
    auto function_die = std::next(top_die->begin(), 3);
    ASSERT_EQ(std::distance(function_die->begin_attr(), function_die->end_attr()), 3);
    // TODO: Finish the testing here
}

TEST_F(NativeSourceTest, FunctionMapping1) {
    const char* elf =
R"(
.text
0 CALL 2
1 HALT
2 PUSH BP
3 MOV BP, SP
4 SUB SP, 2
5 MOV [BP + -1], 5
6 MOV [BP + -2], 6
7 MOV R0, [BP + -1]
8 MOV R1, [BP + -2]
9 ADD R0, R1
10 ADD SP, 2
11 POP BP
12 RET

.debug_line
0: 2
1: 5
2: 6
3: 7
4: 11

.debug_info
DIE_compilation_unit: {
DIE_function: {
    ATTR_name: main,
    ATTR_begin_addr: 2,
    ATTR_end_addr: 12,
    DIE_scope: {
        ATTR_begin_addr: 2,
        ATTR_end_addr: 12,
        DIE_variable: {
            ATTR_name: i,
            ATTR_location: `BASE_REG_OFFSET -1`,
        },
        DIE_variable: {
            ATTR_name: y,
            ATTR_location: `BASE_REG_OFFSET -2`,
        },
    }
}
}
.debug_source
int main(void) {
    int i = 5;
    int y = 10;
    return i + y;
}
)";
    Run(elf);
    native->WaitForDebugEvent();
    for (uint64_t i = 2; i < 12; ++i) {
        ASSERT_TRUE(source.GetFunctionNameByAddress(i)) << i << " is false";
        EXPECT_EQ(*source.GetFunctionNameByAddress(i), "main") << "bad name: " << i;
    }

    EXPECT_EQ(source.GetAddrFunctionByName("main"), 2);

    ASSERT_FALSE(source.GetVariableLocation(*native, "i"));
    native->SetBreakpoint(7);
    native->ContinueExecution();
    ASSERT_TRUE(std::holds_alternative<BreakpointHit>(native->WaitForDebugEvent()));
    auto loc = source.GetVariableLocation(*native, "i");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Offset>(*loc).value, 1021);

    loc = source.GetVariableLocation(*native, "y");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Offset>(*loc).value, 1020);
}

TEST_F(NativeSourceTest, FunctionMapping2) {
auto elf =
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
13      MOV     R1, [SP + -3]
14      PUTNUM  R0
15      PUTNUM  R1
16      XOR     R0, R0        # return 0
17      RET

.debug_lines
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

.debug_info
DIE_compilation_unit: {
DIE_primitive_type: {
    ATTR_name: signed_int,
    ATTR_size: 1,
    ATTR_id: 0,
},
DIE_pointer_type: {
    ATTR_type: 0,
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
        },
        DIE_variable: {
            ATTR_name: y,
            ATTR_type: 1,
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
}

)";
    Run(elf);
    native->WaitForDebugEvent();

    for (uint64_t i = 2; i < 7; ++i) {
        ASSERT_TRUE(source.GetFunctionNameByAddress(i)) << i << " is false";
        EXPECT_EQ(*source.GetFunctionNameByAddress(i), "swap") << "bad name: " << i;
    }
    for (uint64_t i = 7; i < 18; ++i) {
        ASSERT_TRUE(source.GetFunctionNameByAddress(i)) << i << " is false";
        EXPECT_EQ(*source.GetFunctionNameByAddress(i), "main") << "bad name: " << i;
    }

    EXPECT_EQ(source.GetAddrFunctionByName("swap"), 2);
    EXPECT_EQ(source.GetAddrFunctionByName("main"), 7);

    ASSERT_FALSE(source.GetVariableLocation(*native, "i"));
    native->SetBreakpoint(9);
    native->ContinueExecution();

    ASSERT_TRUE(std::holds_alternative<BreakpointHit>(native->WaitForDebugEvent()));
    ASSERT_EQ(native->GetRegister("SP"), 1023);

    // Location
    auto loc = source.GetVariableLocation(*native, "a");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Offset>(*loc).value, 1021);
    // Type
    auto type = source.GetVariableTypeInformation(*native, "a");
    ASSERT_TRUE(type);
    ASSERT_TRUE(std::holds_alternative<PrimitiveType>(*type));
    auto type_info = std::get<PrimitiveType>(*type);
    ASSERT_EQ(type_info.size, 1);
    ASSERT_EQ(type_info.type, PrimitiveType::Type::SIGNED);

    loc = source.GetVariableLocation(*native, "b");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Offset>(*loc).value, 1020);

    native->SetBreakpoint(2);
    native->ContinueExecution();
    native->WaitForDebugEvent();

    loc = source.GetVariableLocation(*native, "x");
    ASSERT_FALSE(loc);
    type = source.GetVariableTypeInformation(*native, "x");
    ASSERT_TRUE(type);
    ASSERT_TRUE(std::holds_alternative<PointerType>(*type));
    const auto& ptr_type = std::get<PointerType>(*type);
    ASSERT_EQ(ptr_type.size, 1);
    ASSERT_TRUE(ptr_type.to);
    ASSERT_TRUE(std::holds_alternative<PrimitiveType>(*ptr_type.to));
    ASSERT_TRUE(std::get<PrimitiveType>(*ptr_type.to).type == PrimitiveType::Type::SIGNED);

    loc = source.GetVariableLocation(*native, "y");
    ASSERT_FALSE(loc);

    loc = source.GetVariableLocation(*native, "tmp");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R0");
}

TEST_F(NativeSourceTest, StructuredTypesBasic) {
    auto elf = 
R"(.text
0   CALL 2 # main()
1   HALT
# main()
2   PUSH BP
3   MOV BP, SP
4   MOV [BP + -2], 1
5   MOV F0, 2.71
6   MOV [BP + -1], F0
7   NRW R0, F0
8   MOV R1, [BP + -2]
9   ADD R0, R1
10  PUTNUM R0
11  POP RBP
12  RET

.debug_line
7: 2
8: 4
9: 4
10: 5
11: 6
12: 10

.debug_info
DIE_compilation_unit: {
DIE_primitive_type: {
    ATTR_name: signed_int,
    ATTR_size: 1,
    ATTR_id: 0,
},
DIE_primitive_type: {
    ATTR_name: float,
    ATTR_size: 1,
    ATTR_id: 1,
},
DIE_structured_type: {
    ATTR_name: coord,
    ATTR_size: 2,
    ATTR_id: 2,
    ATTR_members: {
        0: 1,
        1: 0,
    }
},
DIE_function: {
    ATTR_name: main,
    ATTR_begin_addr: 2,
    ATTR_end_addr: 13,
    DIE_scope: {
        ATTR_begin_addr: 2,
        ATTR_end_addr: 13,
        DIE_variable: {
            ATTR_name: c,
            ATTR_type: 2,
            ATTR_location: `BASE_REG_OFFSET -1`,
        }
    }
}
}

.debug_source
import io.print;

struct coord {
    int x;
    float y;
};

int main() {
    struct coord c;
    c.x = 1;
    c.y = 2.71;
    print(c.x + (int)c.y);
})";
    Run(elf);
    native->WaitForDebugEvent();
    
    native->SetBreakpoint(4);
    native->ContinueExecution();
    native->WaitForDebugEvent();
    auto loc = source.GetVariableLocation(*native, "c");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Offset>(*loc).value, 1021);
    auto type = source.GetVariableTypeInformation(*native, "c");
    ASSERT_TRUE(type);
    ASSERT_TRUE(std::holds_alternative<StructuredType>(*type));
    auto struc = std::get<StructuredType>(*type);
    EXPECT_EQ(struc.name, "coord");
    EXPECT_EQ(struc.size, 2);
    ASSERT_EQ(struc.members.size(), 2);

    auto subtype = struc.members[0];
    ASSERT_EQ(subtype.first, 0);
    ASSERT_TRUE(subtype.second);
    ASSERT_TRUE(std::holds_alternative<PrimitiveType>(*subtype.second));

    subtype = struc.members[1];
    ASSERT_EQ(subtype.first, 1);
    ASSERT_TRUE(subtype.second);
    ASSERT_TRUE(std::holds_alternative<PrimitiveType>(*subtype.second));
}

TEST_F(NativeSourceTest, TestMappingScopes) {
    auto elf =
R"(.text
0   CALL 2
1   HALT
# main()
2   MOV R0, 1
3   MOV R1, 2
4   MOV R2, 3
5   MOV R3, 5
6   ADD R0, R1
7   RET

.debug_info
DIE_compilation_unit: {
DIE_function: {
    ATTR_begin_addr: 2,
    ATTR_end_addr: 8,
    ATTR_name: main,
    DIE_scope: {
        ATTR_begin_addr: 2,
        ATTR_end_addr: 8,
        DIE_variable: {
            ATTR_name: a,
            ATTR_location: `PUSH R0`,
        },
        DIE_scope: {
            ATTR_begin_addr: 3,
            ATTR_end_addr: 7,
            DIE_variable: {
                ATTR_name: b,
                ATTR_location: `PUSH R1`,
            },
            DIE_scope: {
                ATTR_begin_addr: 4,
                ATTR_end_addr: 5,
                DIE_variable: {
                    ATTR_name: a,
                    ATTR_location: `PUSH R2`,
                },
            },
            DIE_scope: {
                ATTR_begin_addr: 5,
                ATTR_end_addr: 6,
                DIE_variable: {
                    ATTR_name: b,
                    ATTR_location: `PUSH R3`,
                },
            },
        },
    },
}
}
.debug_source
int main() {
    int a = 1;
    {
        int b = 2;
        {
            int a = 3;
        }
        {
            int b = 5;
        }
        a += b;
    }
}
)";
    Run(elf);
    native->WaitForDebugEvent();

    for (uint64_t i = 2; i < 8; ++i) {
        ASSERT_TRUE(source.GetFunctionNameByAddress(i)) << i << " is false";
        EXPECT_EQ(*source.GetFunctionNameByAddress(i), "main") << "bad name: " << i;
    }

    ASSERT_FALSE(source.GetVariableLocation(*native, "a"));
    ASSERT_FALSE(source.GetVariableLocation(*native, "b"));
    native->SetBreakpoint(2);
    native->ContinueExecution();
    native->WaitForDebugEvent();

    auto loc = source.GetVariableLocation(*native, "a");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R0");
    ASSERT_FALSE(source.GetVariableLocation(*native, "b"));

    native->PerformSingleStep();
    loc = source.GetVariableLocation(*native, "a");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R0");

    loc = source.GetVariableLocation(*native, "b");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R1");

    native->PerformSingleStep();
    loc = source.GetVariableLocation(*native, "a");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R2");

    loc = source.GetVariableLocation(*native, "b");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R1");

    native->PerformSingleStep();
    loc = source.GetVariableLocation(*native, "a");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R0");

    loc = source.GetVariableLocation(*native, "b");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R3");

    native->PerformSingleStep();
    loc = source.GetVariableLocation(*native, "a");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R0");

    loc = source.GetVariableLocation(*native, "b");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R1");

    native->PerformSingleStep();
    loc = source.GetVariableLocation(*native, "a");
    ASSERT_TRUE(loc);
    ASSERT_EQ(std::get<expr::Register>(*loc).name, "R0");

    ASSERT_FALSE(source.GetVariableLocation(*native, "b"));
}

TEST_F(NativeSourceTest, SourceStepIn) {
    auto elf =
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
12      MOV     R0, [SP + -2] # No debug information about these guys
13      MOV     R1, [SP + -3]
14      PUTNUM  R0
15      PUTNUM  R1
16      XOR     R0, R0        # return 0
17      RET

.debug_line
0: 2
1: 2
3: 5
4: 6
6: 7
7: 7
8: 8
9: 9
10: 16

.debug_source
void swap(int* x, int* y) {
    int tmp = *x;
    *x = *y; // No debug info for this line, should be skipped
    *y = tmp;
}

int main() {
    int a = 3;
    int b = 6;
    swap(&a, &b);
}
)";
    Run(elf);
    native->WaitForDebugEvent();
    native->SetBreakpoint(15); // BP on address which is not mapped to source
    native->SetBreakpoint(7); // BP on line which IS mapped to source (Should be ignored)

    auto e = source.StepIn(*native);
    ASSERT_TRUE(std::holds_alternative<Singlestep>(e));
    ASSERT_EQ(native->GetIP(), 7);

    e = source.StepIn(*native);
    ASSERT_TRUE(std::holds_alternative<Singlestep>(e));
    ASSERT_EQ(native->GetIP(), 8);

    e = source.StepIn(*native);
    ASSERT_TRUE(std::holds_alternative<Singlestep>(e));
    ASSERT_EQ(native->GetIP(), 9);

    e = source.StepIn(*native);
    ASSERT_TRUE(std::holds_alternative<Singlestep>(e));
    ASSERT_EQ(native->GetIP(), 2);

    e = source.StepIn(*native);
    ASSERT_TRUE(std::holds_alternative<Singlestep>(e));
    ASSERT_EQ(native->GetIP(), 5);

    e = source.StepIn(*native);
    ASSERT_TRUE(std::holds_alternative<Singlestep>(e));
    ASSERT_EQ(native->GetIP(), 6);

    e = source.StepIn(*native);
    ASSERT_EQ(native->GetIP(), 15);
    ASSERT_TRUE(std::holds_alternative<BreakpointHit>(e));

    e = source.StepIn(*native);
    ASSERT_TRUE(std::holds_alternative<Singlestep>(e));
    ASSERT_EQ(native->GetIP(), 16);

    e = source.StepIn(*native);
    ASSERT_TRUE(std::holds_alternative<ExecutionEnd>(e));
}
