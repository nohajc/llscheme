//===----- KaleidoscopeJIT.h - A simple JIT for Kaleidoscope ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Contains a simple JIT definition for use in the kaleidoscope tutorials.
//
//===----------------------------------------------------------------------===//

// This code is copied from the LLVM Kaleidoscope tutorial and slightly modified.
// The class is renamed from KaleidoscopeJIT to ScmJIT.

#ifndef LLSCHEME_SCMJIT_HPP
#define LLSCHEME_SCMJIT_HPP

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/RTDyldMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
#include <llvm/IR/Mangler.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/TargetSelect.h>
#include <iostream>
#include "../debug.hpp"

namespace llvm {
    namespace orc {

        class ScmJIT {
        public:
            typedef ObjectLinkingLayer<> ObjLayerT;
            typedef IRCompileLayer<ObjLayerT> CompileLayerT;
            typedef CompileLayerT::ModuleSetHandleT ModuleHandleT;

            ScmJIT()
                    : TM(EngineBuilder().selectTarget()), DL(TM->createDataLayout()),
                      CompileLayer(ObjectLayer, SimpleCompiler(*TM)) {
                llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
            }

            TargetMachine &getTargetMachine() { return *TM; }

            ModuleHandleT addModule(std::shared_ptr<Module> M) {
                // We need a memory manager to allocate memory and resolve symbols for this
                // new module. Create one that resolves symbols by looking back into the
                // JIT.
                auto Resolver = createLambdaResolver(
                        [&](const std::string &Name) {
                            if (auto Sym = findMangledSymbol(Name))
                                return RuntimeDyld::SymbolInfo(Sym.getAddress(), Sym.getFlags());
                            return RuntimeDyld::SymbolInfo(nullptr);
                        },
                        [](const std::string &S) { return nullptr; });
                auto H = CompileLayer.addModuleSet(singletonSet(std::move(M)),
                                                   make_unique<SectionMemoryManager>(),
                                                   std::move(Resolver));

                ModuleHandles.push_back(H);
                return H;
            }

            void removeModule(ModuleHandleT H) {
                ModuleHandles.erase(
                        std::find(ModuleHandles.begin(), ModuleHandles.end(), H));
                CompileLayer.removeModuleSet(H);
            }

            JITSymbol findSymbol(const std::string Name) {
                return findMangledSymbol(mangle(Name));
            }

        private:

            std::string mangle(const std::string &Name) {
                std::string MangledName;
                {
                    raw_string_ostream MangledNameStream(MangledName);
                    Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
                }
                return MangledName;
            }

            template <typename T> static std::vector<T> singletonSet(T t) {
                std::vector<T> Vec;
                Vec.push_back(std::move(t));
                return Vec;
            }

            JITSymbol findMangledSymbol(const std::string &Name) {
                // Search modules in reverse order: from last added to first added.
                // This is the opposite of the usual search order for dlsym, but makes more
                // sense in a REPL where we want to bind to the newest available definition.
                for (auto H : make_range(ModuleHandles.rbegin(), ModuleHandles.rend()))
                    if (auto Sym = CompileLayer.findSymbolIn(H, Name, true))
                        return Sym;

                // If we can't find the symbol in the JIT, try looking in the host process.
                if (auto SymAddr = RTDyldMemoryManager::getSymbolAddressInProcess(Name))
                    return JITSymbol(SymAddr, JITSymbolFlags::Exported);

                return nullptr;
            }

            std::unique_ptr<TargetMachine> TM;
            const DataLayout DL;
            ObjLayerT ObjectLayer;
            CompileLayerT CompileLayer;
            std::vector<ModuleHandleT> ModuleHandles;
        };

    } // End namespace orc.
} // End namespace llvm



#endif //LLSCHEME_SCMJIT_HPP
