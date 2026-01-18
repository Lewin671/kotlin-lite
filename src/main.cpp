#include <iostream>
#include <string>
#include "compiler.hpp"

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <source_file> [options]\n"
              << "Options:\n"
              << "  -o <file>     Write output binary to <file>\n"
              << "  --dump-ir     Dump the custom SSA IR\n"
              << "  --dump-llvm   Dump the generated LLVM IR\n"
              << "  --run         Compile and run the program (default if no -o)\n"
              << "  --help        Show this help message\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    kotlin_lite::CompileOptions options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dump-ir") {
            options.dumpIR = true;
        } else if (arg == "--dump-llvm") {
            options.dumpLLVM = true;
        } else if (arg == "--run") {
            options.shouldRun = true;
        } else if (arg == "-o" && i + 1 < argc) {
            options.outputFile = argv[++i];
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg.substr(0, 1) != "-") {
            options.inputFile = arg;
        }
    }

    if (options.inputFile.empty()) {
        std::cerr << "Error: No input file specified.\n";
        printUsage(argv[0]);
        return 1;
    }

    // If no specific dump flag and no output file, default to run
    if (!options.dumpIR && !options.dumpLLVM && options.outputFile.empty()) {
        options.shouldRun = true;
    }

    kotlin_lite::Compiler compiler;
    return compiler.compile(options);
}
