(define (even n)
  (if (= n 0)
	 #t
	 (odd (- n 1))))

(define (odd n)
  (if (= n 0)
	 #f
	 (if (= n 1)
      #t
		(even (- n 1)))))

(displayln (even 0))
(displayln (odd 0))
(displayln (even 1))
(displayln (odd 1))
(displayln (even 2))
(displayln (odd 2))
(displayln (even 3))
(displayln (odd 3))
