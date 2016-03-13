#include "../include/ast_visitor.hpp"
#include "../include/ast.hpp"

namespace llscm {
    any_ptr AstVisitor::visit(const ScmProg * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmObj * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmInt * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmFloat * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmTrue * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmFalse * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmNull * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmStr * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmSym * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmRef * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmCons * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmFunc * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmCall * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmDefineVarSyntax * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmDefineFuncSyntax * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmLambdaSyntax * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmQuoteSyntax * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmIfSyntax * node) const {
        return node;
    }

    any_ptr AstVisitor::visit(const ScmLetSyntax * node) const {
        return node;
    }
}
