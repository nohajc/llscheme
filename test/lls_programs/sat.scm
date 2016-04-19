; CNF Grammar
; form = ("(" clause ")", form_rest) | literal, form_rest
; formrest = ("&", form) | ""
; clause = literal, clause_rest
; clause_rest = ("|", clause) | ""
; literal = ("!", var) | var

; Example:
; input: (a | b | c) & (!a | !c) & b
; parsed: (lambda (a b c) (list (or a b c) (or (not a) (not c)) (or b)))

(define print display)

(define ns (make-base-namespace))

(define (read-liter tok)
  (if (string=? (car tok) "!")
	 (cons (list 'not (string->symbol (cadr tok))) (cddr tok))
	 (cons (string->symbol (car tok)) (cdr tok))))

(define (read-clause-rest tok)
  (if (null? tok)
	 (list null)
	 (if (string=? (car tok) "|")
      (read-clause (cdr tok))
	   (cons null tok))))

(define (read-clause tok)
  (let ((lit-t (read-liter tok)))
	 (let ((clr-t (read-clause-rest (cdr lit-t))))
		(cons (cons (car lit-t) (car clr-t)) (cdr clr-t)))))

(define (read-form-rest tok)
  (if (null? tok)
	 (list null)
	 (if (string=? (car tok) "&")
	   (read-form (cdr tok))
      (cons null tok))))

(define (read-form tok)
  (if (string=? (car tok) "(")
	 (let ((cl-t (read-clause (cdr tok))))
		(let ((fr-t (read-form-rest (cddr cl-t))))
		  (cons (cons (cons 'or (car cl-t)) (car fr-t)) (cdr fr-t))))
	 (let ((lit-t (read-liter tok)))
		(let ((fr-t (read-form-rest (cdr lit-t))))
        (cons (cons (cons 'or (list (car lit-t))) (car fr-t)) (cdr fr-t))))))

(define (read-formula tok)
  (cons 'list (car (read-form tok))))

(define (sep-kw str kw)
  (string-replace str (car kw) (string-append (string-append " " (car kw)) " ")))

(define (applyrec fn arg next acc)
  (if (null? arg)
	 acc
	 (applyrec fn (next arg) next (fn acc arg))))

(define (sep-keywords str kws)
  (applyrec
	 sep-kw
	 kws
	 (lambda (lst) (cdr lst))
	 str))

(define operators '("(" ")" "&" "|" "!"))

(define (var? a)
  (null? (filter (lambda (x) (string=? x a)) operators)))

(define (randvals len)
  (if (zero? len)
    null
   (cons (zero? (random 2)) (randvals (- len 1)))))

(define (satisfied-clauses-num fn vals)
  (length (filter identity (apply fn vals))))

(define (binary b)
  (if b 1 0))

(define (print-solution vals vars)
  (display "Satisfiable with: ")
  (map
    (lambda (x)
	   (print (car x))
		(display " = ")
		(print (binary (cadr x)))
		(display "  "))
	 (zip vars vals))
  (newline)
  (newline))

(define (print-nosolution)
  (display "No solution found.")
  (newline)
  (newline))

(define (best-flip fn vals varnum)
  (randvals varnum)) ; TODO: find the best flip

(define max_tries 100)

(define (try-assignment fn vals vars varnum clnum num_tries)
  (if (zero? num_tries)
    (print-nosolution)
    (let ((satnum (satisfied-clauses-num fn vals)))
      (if (= satnum clnum)
        (print-solution vals vars)
        (try-assignment fn (best-flip fn vals varnum) vars varnum clnum (- num_tries 1))))))

(define (solve form vars varnum clnum)
  (let ((fn (eval form ns)))
	 (print form)
    (newline)
	 (try-assignment fn (randvals varnum) vars varnum clnum max_tries)))

(define (read-input f)
  (let ((ln (read-line f)))
	 (if (not (eof-object? ln))
		 (let ((tokens
        (string-split
          (sep-keywords ln operators))))
		  (let ((formlst (read-formula tokens))
				  (vars (remove-duplicates (map string->symbol (filter var? tokens)))))
			 (solve (list 'lambda vars formlst) vars (length vars) (length (cdr formlst)))
        (read-input f)))
     null)))

;------------------------------------------------------------
(define argv (current-command-line-arguments))
(if (not (= 1 (vector-length argv)))
  (exit 1)
  (let ()
    (define in_fname (vector-ref argv 0))
    (define in_f (open-input-file in_fname))
    (read-input in_f)
    (close-input-port in_f)))
