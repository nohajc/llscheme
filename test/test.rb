#!/usr/bin/env ruby

require_relative "colors"

SCMC = File.join(__dir__, "../bin/Debug/schemec - 2>/dev/null")

class Test
	def initialize(test_file)
		@test_count = 0
		@failed = 0
		instance_eval(test_file.read)

		puts "\n#{@test_count} tests, #{@failed} failures".bold.light_yellow
	end

	def test_out(test_name, input_str, expected_output)
		output = `echo "#{input_str}" | #{SCMC}`
		print "#{test_name}: "
		@test_count += 1
		if output == expected_output
			puts "OK".bold.green
		else
			puts "FAIL".bold.red
			@failed += 1
			#puts "#{output.chars}"
			#puts "#{expected_output.chars}"
		end
	end
end

Test.new(ARGF)
