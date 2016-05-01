#!/usr/bin/env ruby

def rand_mat_pair(c, m, n, o)
	ret = ""
	c.times do
		ret << (([[lambda {rand 100}] * n]) * m).map{|r| r.map{|e| e.call}}.to_s
		ret << "\n"
		ret << (([[lambda {rand 100}] * o]) * n).map{|r| r.map{|e| e.call}}.to_s
		ret << "\n"
	end
	ret.gsub("[", "(").gsub("]", ")").gsub(",", "")
end

puts "#{rand_mat_pair(ARGV[0].to_i, ARGV[1].to_i, ARGV[2].to_i, ARGV[3].to_i)}"
