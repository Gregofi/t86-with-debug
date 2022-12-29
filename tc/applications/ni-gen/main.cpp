#include <cstdlib>
#include <iostream>
#include <cstring>

#include "tinyC/ast.h"
#include "tinyC/frontend.h"
#include "tinyC/backend/codegen.h"
#include "tinyC/backend/optimizations.h"
#include "tinyC/backend/tiny86_gen.h"
#include "tinyC/backend/register_allocator.h"

using namespace tiny;
using namespace tinyc;
using namespace colors;

char const* tests[] = {
        R"(
        int foo() {
            return 60;
        }

        int main() {
            return foo();
        }
    )", // 0
};


/* =============================================== */

const char* usage = R"(
usage: ni-gen [options] file

Options:
    -o <file>        Place the output into <file>
    -x               Output generated assembly.
    -ir              Output generated IR code.
    -r               Print the 'main' exit code.
    -a               Output AST.
    -OS              Apply the strength reduction optimization.
    -OI              Apply inlining optimization.
    -ODF             Apply the Remove dead functions optimization.
    -ODC             Apply the Remove dead code optimization.
)";

int typecheckerTest(size_t test) {
    // For now, just check that the guy doesn't fail
    try {
        Frontend front;
        std::unique_ptr<AST> ast = front.parse(tests[test]);
        Typechecker tp;
        tp.visit(ast.get());
    } catch (const ParserError& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

void print_usage() {
    std::cout << usage << std::endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    bool dump_assembly = false, dump_ast = false, dump_ir = false, exit_print = false,
         inline_calls = false, reduce_by_strength = false, remove_dead_funcs = false,
         remove_dead_code = false;
    std::string output_file;
    std::string input_file;

    /// ---------------------
    /// Command line args
    /// ---------------------
    if (argc < 2) {
        print_usage();
    }
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-o")) {
            i += 1;
            if (i >= argc) {
                print_usage();
            }
            output_file = argv[i];
        } else if (!strcmp(argv[i], "-x")) {
            dump_assembly = true;
        } else if (!strcmp(argv[i], "-ir")) {
            dump_ir = true;
        } else if (!strcmp(argv[i], "-r")) {
            exit_print = true;
        } else if (!strcmp(argv[i], "-a")) {
            dump_ast = true;
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else if (!strcmp(argv[i], "-OI")) {
            inline_calls = true;
        } else if (!strcmp(argv[i], "-OS")) {
            reduce_by_strength = true;
        } else if (!strcmp(argv[i], "-ODF")) {
            remove_dead_funcs = true;
        } else if (!strcmp(argv[i], "-ODC")) {
            remove_dead_code = true;
        } else {
            print_usage();
        }
    }

    if (input_file.empty()) {
        print_usage();
    }

    /// Type checker tests
    for (size_t i = 0; i < sizeof(tests) / sizeof(*tests); ++i) {
        typecheckerTest(i);
    }

    initializeTerminal();
    try {
        Frontend front;
        std::unique_ptr<AST> ast = front.parseFile(input_file);
        if (dump_ast) {
            ast->dump();
        }

        // typecheck
        Typechecker tp;
        tp.visit(ast.get());

        // translate to IR
        ColorPrinter p(std::cerr);
        Context ctx;
        Codegen cg(&ctx);
        cg.visit(ast.get());

        // optimize
        std::vector<std::unique_ptr<Optimization>> opts;
        if (remove_dead_funcs) {
            opts.emplace_back(std::make_unique<DeadCallsRemoval>());
        }
        if (reduce_by_strength) {
            opts.emplace_back(std::make_unique<StrengthReduction>());
        }
        if (remove_dead_code) {
            opts.emplace_back(std::make_unique<DeadCodeRemoval>());
        }
        if (inline_calls) {
            // TODO
        }
        for (auto& opt : opts) {
            opt->optimize(ctx);
        }

        if (dump_ir) {
            ctx.dump(p);
        }

        // translate to target
        Tiny86Gen g;
        auto program = g.generate(ctx, exit_print);

        // Dump the assembly
        if (dump_assembly) {
            size_t cnt = 0;
            fmt::print(".text\n");
            for (const auto& x: program) {
                fmt::print("{:d} {:s}\n", cnt++, x.toString());
            }
        }

    }
    catch (std::exception const& e) {
        std::cerr << color::red << "ERROR: " << color::reset << e.what() << std::endl;
    } catch (...) {
        std::cerr << color::red << "UNKNOWN ERROR. " << color::reset << std::endl;
    }
    return EXIT_SUCCESS;
}