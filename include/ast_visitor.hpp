#ifndef LLSCHEME_AST_VISITOR_HPP
#define LLSCHEME_AST_VISITOR_HPP

#include "any_ptr.hpp"

namespace llscm {
    class ScmProg;
    class ScmObj;
    class ScmInt;
    class ScmFloat;
    class ScmTrue;
    class ScmFalse;
    class ScmNull;
    class ScmStr;
    class ScmSym;
    class ScmRef;
    class ScmCons;
    class ScmFunc;
    class ScmCall;
    class ScmDefineVarSyntax;
    class ScmDefineFuncSyntax;
    class ScmLambdaSyntax;
    class ScmQuoteSyntax;
    class ScmIfSyntax;
    class ScmLetSyntax;
    // TODO: add the rest

    class AstVisitor {
    public:
        virtual any_ptr visit(ScmProg *node);
        virtual any_ptr visit(ScmObj * node);
        virtual any_ptr visit(ScmInt * node);
        virtual any_ptr visit(ScmFloat * node);
        virtual any_ptr visit(ScmTrue * node);
        virtual any_ptr visit(ScmFalse * node);
        virtual any_ptr visit(ScmNull * node);
        virtual any_ptr visit(ScmStr * node);
        virtual any_ptr visit(ScmSym * node);
        virtual any_ptr visit(ScmRef * node);
        virtual any_ptr visit(ScmCons * node);
        virtual any_ptr visit(ScmFunc * node);
        virtual any_ptr visit(ScmCall * node);
        virtual any_ptr visit(ScmDefineVarSyntax * node);
        virtual any_ptr visit(ScmDefineFuncSyntax * node);
        virtual any_ptr visit(ScmLambdaSyntax * node);
        virtual any_ptr visit(ScmQuoteSyntax * node);
        virtual any_ptr visit(ScmIfSyntax * node);
        virtual any_ptr visit(ScmLetSyntax * node);
    };

    class VisitableObj {
    public:
        virtual any_ptr accept(AstVisitor * visitor) = 0;
    };

    template<typename T>
    class Visitable: public VisitableObj {
    public:
        virtual any_ptr accept(AstVisitor * visitor) {
            return visitor->visit(static_cast<T*>(this));
        }
    };
}

#endif //LLSCHEME_AST_VISITOR_HPP