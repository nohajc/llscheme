#include <sstream>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/DynamicLibrary.h>
#include "../include/environment.hpp"
#include "../include/debug.hpp"
#include "../include/codegen.hpp"
#include "../include/fs_helpers.hpp"
#include "../include/lib_reader.hpp"

namespace llscm {
    using namespace std;
    using namespace llvm;

    void initGlobalEnvironment(ScmEnv * env, void * lib_blob) {
        env->set("cons", make_shared<ScmConsFunc>());
        env->set("car", make_shared<ScmCarFunc>());
        env->set("cdr", make_shared<ScmCdrFunc>());
        env->set("+", make_shared<ScmPlusFunc>());
        env->set("-", make_shared<ScmMinusFunc>());
        env->set("null?", make_shared<ScmNullFunc>());
        env->set(">", make_shared<ScmGtFunc>());
        env->set("=", make_shared<ScmNumEqFunc>());
        env->set("*", make_shared<ScmTimesFunc>());
        env->set("/", make_shared<ScmDivFunc>());
        env->set("display", make_shared<ScmDisplayFunc>());
        env->set("current-command-line-arguments", make_shared<ScmCmdArgsFunc>());
        env->set("vector-length", make_shared<ScmVecLenFunc>());
        env->set("vector-ref", make_shared<ScmVecRefFunc>());
        env->set("apply", make_shared<ScmApplyFunc>());
        env->set("length", make_shared<ScmLengthFunc>());

        env->set("make-base-namespace", make_shared<ScmFunc>(0, RuntimeSymbol::make_base_nspace));
        env->set("current-namespace", make_shared<ScmFunc>(ArgsAnyCount, RuntimeSymbol::current_nspace));
        env->set("eval", make_shared<ScmFunc>(2, RuntimeSymbol::eval));
        env->set("read", make_shared<ScmFunc>(0, RuntimeSymbol::read));
        env->set("eof-object?", make_shared<ScmFunc>(1, RuntimeSymbol::is_eof));
        env->set("list", make_shared<ScmFunc>(ArgsAnyCount, RuntimeSymbol::list));
        env->set("string->symbol", make_shared<ScmFunc>(1, RuntimeSymbol::string_to_symbol));
        env->set("string=?", make_shared<ScmFunc>(2, RuntimeSymbol::string_equals));
        env->set("string-append", make_shared<ScmFunc>(2, RuntimeSymbol::string_append));
        env->set("string-replace", make_shared<ScmFunc>(3, RuntimeSymbol::string_replace));
        env->set("string-split", make_shared<ScmFunc>(1, RuntimeSymbol::string_split));
        env->set("open-input-file", make_shared<ScmFunc>(1, RuntimeSymbol::open_input_file));
        env->set("close-input-port", make_shared<ScmFunc>(1, RuntimeSymbol::close_input_port));
        env->set("read-line", make_shared<ScmFunc>(1, RuntimeSymbol::read_line));
        env->set("equal?", make_shared<ScmFunc>(2, RuntimeSymbol::equal));
        env->set("exit", make_shared<ScmFunc>(1, RuntimeSymbol::exit));
        env->set("random", make_shared<ScmFunc>(1, RuntimeSymbol::random));

        // TODO: eq?

        LibReader dylib;
        Metadata input_meta;
        void * metainfo_blob;

        // Load other symbols from the runtime library (those implemented in Scheme - without c headers)
        if (lib_blob) {
            metainfo_blob = lib_blob;
            D(cerr << "lib blob address = " << lib_blob << endl);
        }
        else {
            D(cerr << "loading library using our custom reader" << endl);
            auto res = getLibraryPath("libllscmrt.so");
            if (!res.second) {
                cerr << "Runtime library not found" << endl;
                return;
            }

            if (!dylib.load(res.first)) {
                return;
            }

            metainfo_blob = dylib.getAddressOfSymbol("__llscheme_metainfo__");
            if (!metainfo_blob) {
                return;
            }
        }

        if (!input_meta.loadFromBlob(metainfo_blob)) {
            cerr << "Error: Invalid metadata in the runtime library." << endl;
            exit(EXIT_FAILURE);
        }

        input_meta.foreachRecord([env](FunctionInfo *rec) {
            D(cerr << "Found function \"" << rec->name << "\" with " << rec->argc << " args." << endl);
            // Add the function into environment
            env->set(rec->name, make_shared<ScmFunc>(rec->argc, rec->name));
        });
    }

    shared_ptr<ScmEnv> createGlobalEnvironment(ScmProg & prog) {
        shared_ptr<ScmEnv> env = make_shared<ScmEnv>(&prog);

        initGlobalEnvironment(env.get());
        return env;
    }

    int ScmEnv::GlobalLevel = -2;

    ScmEnv::ScmEnv(ScmProg * p, P_ScmEnv penv): prog(p), parent_env(penv) {
        if (!penv) {
            top_level_env = this;
        }
        else {
            top_level_env = penv->top_level_env;
        }
        err_flag = false;
        if (prog) {
            prog_begin = prog->begin();
        }
        context = nullptr;
        link_lib = false;
    }

    void ScmEnv::setProg(ScmProg & p) {
        prog = &p;
        prog_begin = prog->begin();
    }

    P_ScmObj ScmEnv::get(P_ScmObj k, ScmLoc * loc) {
        ScmSym * sym = dynamic_cast<ScmSym*>(k.get());
        return get(sym, loc);
    }

    P_ScmObj ScmEnv::get(ScmSym * sym, ScmLoc * loc) {
        bool find_func = false;
        ScmFunc * func = nullptr;
        if (loc) {
            find_func = true;
            if (context && context->t == T_FUNC) {
                func = DPC<ScmFunc>(context).get();
            }

            if (!*loc) {
                *loc = make_shared<pair<int, ScmFunc*>>(-1, func);
            }

            if (func) {
                (*loc)->first += 1;
                (*loc)->second = func;
            }

            if (this == top_level_env) {
                (*loc)->first = GlobalLevel;
                find_func = false;
            }
        }

        auto elem_it = binding.find(*sym);
        if (elem_it == binding.end()) {
            if (parent_env) {
                return parent_env->get(sym, loc);
            }
            return nullptr;
        }

        if (!func && find_func && *loc) {
            // We're inside a let block
            D(cerr << "FIND_FUNC" << endl);
            ScmEnv * p_env = this;
            while (p_env) {
                if (p_env == top_level_env) {
                    (*loc)->first = GlobalLevel;
                    break;
                }
                if (p_env->context && p_env->context->t == T_FUNC) {
                    (*loc)->first += 1;
                    (*loc)->second = DPC<ScmFunc>(p_env->context).get();;
                    break;
                }
                p_env = p_env->parent_env.get();
            }
        }
        // If valid pointer given in def_env, return
        // the environment where we've found the symbol binding.
        //if (def_env) *def_env = shared_from_this();
        return elem_it->second;
    }

    bool ScmEnv::set(P_ScmObj k, P_ScmObj obj) {
        ScmSym * sym = dynamic_cast<ScmSym*>(k.get());
        if (!sym) {
            return false;
        }
        binding[*sym] = obj;
        return true;
    }

    bool ScmEnv::set(const string & k, P_ScmObj obj) {
        unique_ptr<ScmSym> sym = make_unique<ScmSym>(k);
        if (!sym) {
            return false;
        }
        binding[*sym] = obj;
        return true;
    }

    void ScmEnv::error(const string &msg) {
        cout << "Error: " << msg << endl;
        top_level_env->err_flag = err_flag = true;
    }

    string ScmEnv::getUniqID(const string & name) {
        return top_level_env->namegen.getUniqID(name);
    }

    string ScmNameGen::getUniqID(const string & name) {
        auto kv = uniq_id.find(name);
        stringstream ss;
        if (kv != uniq_id.end()) {
            kv->second++;
            ss << name << kv->second;
        }
        else {
            uniq_id[name] = 0;
            ss << name << 0;
        }
        return ss.str();
    }


    ScmFunc * ScmEnv::defInFunc() {
        ScmFunc * func = nullptr;
        ScmEnv * p_env = this;
        while (p_env) {
            if (p_env->context && p_env->context->t == T_FUNC) {
                func = DPC<ScmFunc>(p_env->context).get();;
                break;
            }
            p_env = p_env->parent_env.get();
        }
        return func;
    }

    void ScmEnv::setGlobalsAsExternal() {
        for (auto & b: binding) {
            P_ScmObj & obj = b.second;
            obj->is_extern = true;
            obj->IR_val = nullptr;
        }
    }

    void ScmEnv::checkUnRefs() {
        auto & refs = getUnRefs();
        for (auto r: refs) {
            error(r.first.val + " is not defined.");
        }
        refs.clear();
        evalAgain().clear();
    }
}

