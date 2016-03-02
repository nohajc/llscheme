#!/usr/bin/env ruby

require_relative "colors"

SCMC = "../bin/Debug/schemec - 2>/dev/null"

def test_out(test_name, input_str, expected_output)
	output = `echo "#{input_str}" | #{SCMC}`
	print "#{test_name}: "
	if output == expected_output
		puts "OK".green
	else
		puts "FAIL".red
		#puts "#{output.chars}"
		#puts "#{expected_output.chars}"
	end
end

test_out("Test empty program", "", "Error: Program is empty.\n")

test_out("Test incomplete list 1", "(", "Error: Reached EOF while parsing a list.\n")
test_out("Test incomplete list 2", "(foo", "Error: Reached EOF while parsing function call.\n")
test_out("Test incomplete list 3", "(foo bar", "Error: Reached EOF while parsing function call.\n")

test_out("Test incomplete definition 1", "(define", "Error: Reached EOF while parsing a definition.\n")
test_out("Test incomplete definition 2", "(define (", "Error: Missing function name in definition.\n")

test_out("Test invalid function definition 1", "(define (1) foo)", "Error: Missing function name in definition.\n")
test_out("Test invalid function definition 2", "(define () bar)", "Error: Missing function name in definition.\n")

test_out("Test invalid variable name in definition", "(define 3.14", "Error: Expected symbol as first argument of define.\n")

test_out("Test missing expression in definition 1", "(define foo)", "Error: Missing expression in variable definition.\n")
test_out("Test missing expression in definition 2", "(define foo", "Error: Expected expression.\n")

test_out("Test incomplete argument list", "(define (foo bar", "Error: Reached EOF while parsing a list.\n")

test_out("Test invalid expression in argument list", "(define (foo \\\"bar\\\") #t)", "Error: Invalid expression in argument list. Only symbols are allowed.\n")
test_out("Test invalid expression in argument list 2", "(define (foo \'bar\') #t)", "Error: Invalid expression in argument list. Only symbols are allowed.\n")

test_out("Test incomplete binding list", "(let ((a)) #f)", "Error: Binding list must have exactly two elements: id, expression.\n")

test_out("Test invalid expression in binding list", "(let ((2 foo)) null)", "Error: First element of binding list must be a symbol.\n")

test_out("Test incomplete body 1", "(let ((foo 2)))", "Error: Missing expression in a body.\n")
test_out("Test incomplete body 2", "(let ((foo 2))", "Error: Reached EOF while parsing a body.\n")
test_out("Test incomplete body 3", "(let ((foo 2)) foo", "Error: Reached EOF while parsing a body.\n")