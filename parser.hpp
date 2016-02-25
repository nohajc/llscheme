#include "types.hpp"

/*

prog = form { form }
form = def | expr
def = "(" "define" sym expr ")" | "(" "define" "(" symlist ")" body ")" | "(" "let" "(" bindlist ")" body ")"
expr = atom | ( "(" list ")" )
atom = str | sym | int | float | true | false | null
symlist = { sym }
bindlist = { "(" sym expr ")" }
list = { expr }
body = { def } expr { expr }


*/

namespace llscm {
	ScmObj * NT_Prog();
}