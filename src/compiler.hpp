#pragma once
#include <string>
#include <vector>
#include <memory>

namespace kotlin_lite {
    struct CompileOptions {
        std::string inputFile;
        std::string outputFile;
        bool dumpIR = false;
        bool dumpLLVM = false;
        bool shouldRun = false;
    };

    class Compiler {
    public:
        Compiler() = default;
        ~Compiler() = default;
        
        // Main compilation method
        int compile(const CompileOptions& options);
        
    private:
        std::string getRuntimePath() const;
    };
}
