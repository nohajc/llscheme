#ifndef LLSCHEME_COMPILER_HPP
#define LLSCHEME_COMPILER_HPP

#include <memory>
//#include <llvm/IR/Module.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/ADT/Triple.h>

namespace llscm {
    using namespace std;
    using namespace llvm;

    class ScmCompiler {
        shared_ptr<Module> module;
        LLVMContext & context;

        unique_ptr<tool_output_file>
        GetOutputStream(const char *TargetName, Triple::OSType OS,
                                     const char *ProgName);

        int compileModule(char **, LLVMContext &);
    public:
        ScmCompiler(shared_ptr<Module> mod, LLVMContext & c): module(mod), context(c) {}
        int run();
    };
}

#endif //LLSCHEME_COMPILER_HPP
