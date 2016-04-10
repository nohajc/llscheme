; Library functions implementation

(define (not obj) (if obj #f #t))
(define (zero? n) (= n 0))

(define (cadr l) (car (cdr l)))
(define (cddr l) (cdr (cdr l)))

(define (list-ref lst idx)
  (if (null? lst)
	 null
	 (if (zero? idx)
	   (car lst)
	   (list-ref (cdr lst) (- idx 1)))))

(define (>= a b) (or (> a b) (= a b)))
(define (<= a b) (not (> a b)))
(define (< a b) (not (>= a b)))

(define (append a b)
  (if (null? a) b (cons (car a) (append (cdr a) b))))

(define (map fn lst)
  (if (null? lst)
    null
    (cons (fn (car lst)) (map fn (cdr lst)))))

(define (filter pred lst)
  (if (null? lst)
    null
    (if (pred (car lst))
      (cons (car lst) (filter pred (cdr lst)))
      (filter pred (cdr lst)))))

(define (compose1 fn1 fn2)
  (lambda (x) (fn1 (fn2 x))))

(define (displayln obj)
  (display obj)
  (display "\n"))

; This is for debugging.
; We just want to see if the library was loaded.
;(display "Hello library!\n")
