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
        virtual any_ptr visit(const ScmProg * node) const;
        virtual any_ptr visit(const ScmObj * node) const;
        virtual any_ptr visit(const ScmInt * node) const;
        virtual any_ptr visit(const ScmFloat * node) const;
        virtual any_ptr visit(const ScmTrue * node) const;
        virtual any_ptr visit(const ScmFalse * node) const;
        virtual any_ptr visit(const ScmNull * node) const;
        virtual any_ptr visit(const ScmStr * node) const;
        virtual any_ptr visit(const ScmSym * node) const;
        virtual any_ptr visit(const ScmRef * node) const;
        virtual any_ptr visit(const ScmCons * node) const;
        virtual any_ptr visit(const ScmFunc * node) const;
        virtual any_ptr visit(const ScmCall * node) const;
        virtual any_ptr visit(const ScmDefineVarSyntax * node) const;
        virtual any_ptr visit(const ScmDefineFuncSyntax * node) const;
        virtual any_ptr visit(const ScmLambdaSyntax * node) const;
        virtual any_ptr visit(const ScmQuoteSyntax * node) const;
        virtual any_ptr visit(const ScmIfSyntax * node) const;
        virtual any_ptr visit(const ScmLetSyntax * node) const;
    };

    class VisitableObj {
    public:
        virtual any_ptr accept(AstVisitor * visitor) const = 0;
    };

    template<typename T>
    class Visitable: public VisitableObj {
    public:
        virtual any_ptr accept(AstVisitor * visitor) const {
            return visitor->visit(static_cast<const T*>(this));
        }
    };
}

#endif //LLSCHEME_AST_VISITOR_HPP
