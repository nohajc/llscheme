#include "../include/ast_visitor.hpp"
#include "../include/ast.hpp"

namespace llscm {
    any_ptr AstVisitor::visit(ScmProg *node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmObj * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmInt * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmFloat * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmTrue * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmFalse * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmNull * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmStr * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmSym * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmRef * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmCons * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmFunc * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmCall * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmDefineVarSyntax * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmDefineFuncSyntax * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmLambdaSyntax * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmQuoteSyntax * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmIfSyntax * node) {
        return node;
    }

    any_ptr AstVisitor::visit(ScmLetSyntax * node) {
        return node;
    }
}
