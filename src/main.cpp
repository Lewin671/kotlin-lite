#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "ir/ir_generator.hpp"
#include "codegen/llvm_codegen.hpp"
#include <llvm/Support/raw_ostream.h>

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <source_file> [options]\n"
              << "Options:\n"
              << "  --dump-ir     Dump the custom SSA IR\n"
              << "  --dump-llvm   Dump the generated LLVM IR\n"
              << "  --run         Compile and run the program (default)\n"
              << "  --help        Show this help message\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile = "";
    bool dumpIR = false;
    bool dumpLLVM = false;
    bool shouldRun = false; // Default to false, will set to true if no other specific dump flags are set or --run is present

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-ir") dumpIR = true;
        else if (arg == "--dump-llvm") dumpLLVM = true;
        else if (arg == "--run") shouldRun = true;
        else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg.substr(0, 1) != "-") {
            inputFile = arg;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified.\n";
        printUsage(argv[0]);
        return 1;
    }

    // If no specific dump flag is set, default to run
    if (!dumpIR && !dumpLLVM) shouldRun = true;

    // 1. Read source file
    std::ifstream file(inputFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << inputFile << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    try {
        // 2. Lexing
        kotlin_lite::Lexer lexer(source);
        auto tokens = lexer.tokenize();

        // 3. Parsing
        kotlin_lite::Parser parser(std::move(tokens));
        auto ast = parser.parse();

        // 4. Semantic Analysis
        kotlin_lite::SemanticAnalyzer analyzer;
        analyzer.analyze(*ast);
        if (!analyzer.getErrors().empty()) {
            std::cerr << "Semantic Errors:\n";
            for (const auto& err : analyzer.getErrors()) std::cerr << "  " << err << "\n";
            return 1;
        }

        // 5. IR Generation
        kotlin_lite::ir::IRGenerator irGen;
        auto irMod = irGen.generate(*ast);
        if (dumpIR) {
            std::cout << "--- Custom IR ---\n" << irMod->dump() << "\n";
        }

        // 6. LLVM Codegen
        kotlin_lite::LLVMCodegen llvmCodegen;
        auto llvmMod = llvmCodegen.generate(*irMod);
        if (dumpLLVM) {
            std::cout << "--- LLVM IR ---\n";
            llvmMod->print(llvm::outs(), nullptr);
        }

        if (shouldRun) {
            // Write LLVM IR to temporary file
            std::error_code ec;
            llvm::raw_fd_ostream dest("output.ll", ec);
            if (ec) {
                std::cerr << "Could not open output.ll: " << ec.message() << std::endl;
                return 1;
            }
            llvmMod->print(dest, nullptr);
            dest.flush();
            dest.close();

            // Find runtime path relative to executable or project
            // For now, assume we are in 'build' and runtime is in '../src/runtime/runtime.c'
            std::string runtimePath = "../src/runtime/runtime.c";
            if (!std::filesystem::exists(runtimePath)) {
                // Try current dir if not in build
                runtimePath = "src/runtime/runtime.c";
            }

            std::string cmd = "clang -Wno-override-module output.ll " + runtimePath + " -o program && ./program";
            int ret = system(cmd.c_str());
            if (ret != 0) {
                // std::cerr << "Execution failed with code: " << ret << std::endl;
                // Note: on some systems, ret might be shifted
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Compilation failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
