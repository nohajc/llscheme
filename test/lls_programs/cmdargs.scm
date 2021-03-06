(define argv (current-command-line-arguments))

(define (print-cmd-args idx num)
  (if (= num 0)
	 null
	 (let ((arg (vector-ref argv idx)))
		(display arg)
		(display "\n")
      (print-cmd-args (+ idx 1) (- num 1)))))

(print-cmd-args 0 (vector-length argv))
