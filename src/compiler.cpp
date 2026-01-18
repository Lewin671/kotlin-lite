#include "compiler.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "ir/ir_generator.hpp"
#include "codegen/llvm_codegen.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <llvm/Support/raw_ostream.h>

namespace kotlin_lite {
    std::string Compiler::getRuntimePath() const {
        if (std::filesystem::exists("../src/runtime/runtime.c")) {
            return "../src/runtime/runtime.c";
        }
        return "src/runtime/runtime.c";
    }

    int Compiler::compile(const CompileOptions& options) {
        // Validate input file
        std::ifstream file(options.inputFile);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << options.inputFile << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        try {
            // 1. Lexing
            Lexer lexer(source);
            auto tokens = lexer.tokenize();

            // 2. Parsing
            Parser parser(std::move(tokens));
            auto ast = parser.parse();

            // 3. Semantic Analysis
            SemanticAnalyzer analyzer;
            analyzer.analyze(*ast);
            if (!analyzer.getErrors().empty()) {
                std::cerr << "Semantic Errors:\n";
                for (const auto& err : analyzer.getErrors()) {
                    std::cerr << "  " << err << "\n";
                }
                return 1;
            }

            // 4. IR Generation
            ir::IRGenerator irGen;
            auto irMod = irGen.generate(*ast);
            if (options.dumpIR) {
                std::cout << "--- Custom IR ---\n" << irMod->dump() << "\n";
            }

            // 5. LLVM Codegen
            LLVMCodegen llvmCodegen;
            auto llvmMod = llvmCodegen.generate(*irMod);
            if (options.dumpLLVM) {
                std::cout << "--- LLVM IR ---\n";
                llvmMod->print(llvm::outs(), nullptr);
            }

            // 6. Compilation
            if (!options.outputFile.empty() || options.shouldRun) {
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

                std::string runtimePath = getRuntimePath();
                std::string binaryName = options.outputFile.empty() ? "./program" : options.outputFile;
                std::string compileCmd = "clang -O3 -Wno-override-module output.ll " + runtimePath + " -o " + binaryName;
                
                int compileRet = system(compileCmd.c_str());
                if (compileRet != 0) {
                    std::cerr << "Compilation failed during linking.\n";
                    return 1;
                }

                if (options.shouldRun) {
                    system(binaryName.c_str());
                } else {
                    std::cout << "Binary generated: " << binaryName << "\n";
                }
            }

            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Compilation failed: " << e.what() << std::endl;
            return 1;
        }
    }
}
