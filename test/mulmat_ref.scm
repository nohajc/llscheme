;#lang racket

(define (zip a b)
  (if (or (null? a) (null? b))
    null
    (cons (list (car a) (car b)) (zip (cdr a) (cdr b)))))


(define row list-ref)

(define (col mat idx)
  (if (null? mat)
    null
	 (cons (list-ref (car mat) idx) (col (cdr mat) idx))))

(define (mul-elem row col)
  (foldl + 0 (map (lambda (pair) (* (car pair) (cadr pair))) (zip row col))))

(define (mul-row row mat2 j colnum)
  (if (= j colnum)
    null
	 (cons (mul-elem row (col mat2 j)) (mul-row row mat2 (+ j 1) colnum))))

(define (mul-rows mat1 mat2 colnum)
  (if (null? mat1)
    null
	 (cons (mul-row (row mat1 0) mat2 0 colnum) (mul-rows (cdr mat1) mat2 colnum))))

(define (mul-mat mat1 mat2)
  (mul-rows mat1 mat2 (length mat1)))


(define (multiply-input)
  (let ((m1 (read)) (m2 (read)))
	 (if (eof-object? m1)
		null
		(let ()
		  (displayln (mul-mat m1 m2))
		  (multiply-input)))))

(multiply-input)
