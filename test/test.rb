#!/usr/bin/env ruby

require "open3"
require_relative "colors"

SCMC = File.join(__dir__, "../bin/Debug/schemec -")

class Test
	def initialize(test_file)
		@test_count = 0
		@failed = 0
		instance_eval(test_file.read)

		puts "\n#{@test_count} tests, #{@failed} failures".bold.light_yellow
	end

	def test_out(test_name, input_str, expected_output)
		stdin, stdout, stderr = Open3.popen3(SCMC)

		stdin.puts(input_str)
		stdin.close

		print "#{test_name}: "
		@test_count += 1
		if stdout.gets == expected_output
			puts "OK".bold.green
		else
			puts "FAIL".bold.red
			@failed += 1
			#puts "#{output.chars}"
			#puts "#{expected_output.chars}"
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
