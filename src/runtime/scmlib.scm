; Library functions implementation

(define (cadr l) (car (cdr l)))
(define (cddr l) (cdr (cdr l)))
(define (zero? n) (= n 0))

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

(define (displayln obj)
  (display obj)
  (display "\n"))

; This is for debugging.
; We just want to see if the library was loaded.
;(display "Hello library!\n")