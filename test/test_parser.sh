#!/usr/bin/env bash

SCMC=../bin/Debug/schemec

# Test empty program
echo "" | $SCMC -

# Test incomplete list
echo "(" | $SCMC -
echo "(foo" | $SCMC -
echo "(foo bar" | $SCMC -

# Test incomplete definition
echo "(define" | $SCMC -
echo "(define (" | $SCMC -
echo "(define (1) foo)" | $SCMC -
echo "(define () bar)" | $SCMC -

# Test invalid var name in definition
echo "(define 3.14" | $SCMC -

# Test missing expression in definition
echo "(define foo)" | $SCMC -
echo "(define foo" | $SCMC -

# Test incomplete arg list
echo "(define (foo bar" | $SCMC -

# Test invalid expression in arg list
echo "(define (foo \"bar\") #t)" | $SCMC -

# Test incomplete bind list
echo "(let ((a)) #f)" | $SCMC -

# Test invalid expression in bind list
echo "(let ((2 foo)) null)" | $SCMC -

# Test incomplete body
echo "(let ((foo 2)))" | $SCMC -
echo "(let ((foo 2))" | $SCMC -
echo "(let ((foo 2)) foo" | $SCMC -

