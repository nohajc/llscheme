#!/usr/bin/env ruby

require "open3"
require_relative "colors"

SCMC = File.join(__dir__, "./schemec - -f null")

class Test
	def initialize(test_file)
		@test_count = 0
		@failed = 0
		instance_eval(test_file.read)

		puts "\n#{@test_count} tests, #{@failed} failures".bold.light_yellow
	end

	def test_out(test_name, input_str, expected_output)
		print "#{test_name}: "
		stdin, stdout, stderr = Open3.popen3(SCMC)

		stdin.puts(input_str)
		stdin.close
		output = stdout.read
		err = stderr.read

		@test_count += 1
		if output == expected_output
			puts "OK".bold.green
			puts "#{err}"
		else
			puts "FAIL".bold.red
			@failed += 1
			puts "Expected: #{expected_output.dump}"
			puts "Returned: #{output.dump}"
		end

		stdout.close
		stderr.close
	end

	def show_out(input_str)
		output = `echo "#{input_str}" | #{SCMC}`
		puts "#{output}"
	end
end

Test.new(ARGF)
