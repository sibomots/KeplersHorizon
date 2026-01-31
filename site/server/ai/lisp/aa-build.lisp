;;;; aa-build.lisp - Build Phase Decision Logic (One Command at a Time)

;;; Warpship costs:
;;;   WG = 5 BP (required for warpship)
;;;   PD, B, S, T, SR = 1 BP each
;;;   M = 3 missiles per 1 BP
;;;
;;; Basic fighter: WG(5) + PD(5) + B(3) + S(2) + T=1 + M(3) = 19 BP

(defparameter *ship-name-counter* 0)

(defun next-ship-name ()
  "Generate next ship name."
  (incf *ship-name-counter*)
  (format nil "ALPHA~D" *ship-name-counter*))

;;; ----------------------------------------------------------------------------
;;; Build Phase Entry
;;; ----------------------------------------------------------------------------

(defparameter *credit-reserve* 50
  "Keep this many credits in reserve for repairs/resupply.")

(defparameter *ship-cost* 19
  "Cost of a basic warpship (WG=5 + PD=5 + B=3 + S=2 + T=1 + M=3).")

(defun should-build-p (slate)
  "Heuristic: Should we build another ship?"
  (let ((credits (slate-credits slate))
        (our-ships (length (slate-own-ships slate)))
        (enemy-ships (length (slate-enemy-ships slate))))
    (and (>= credits (+ *ship-cost* *credit-reserve*))
         (< our-ships (min 5 (max 2 (+ enemy-ships 1)))))))

(defun decide-build-phase (slate)
  "Decide ONE action in build phase. Called repeatedly until NEXT.
   State machine:
   1. Draft with specs (pd>0) -> commit it (bc)
   2. Draft without specs -> set specs (bs)
   3. Ship not deployed -> deploy it (ds)
   4. Should build? -> create draft (bn)
   5. Otherwise -> advance (NEXT)"
  (let ((drafts (slate-drafts slate))
        (ships (slate-own-ships slate)))
    (format t "[LISP] decide-build: drafts=~A ships=~A credits=~A~%"
            (length drafts) (length ships) (slate-credits slate))
    (cond
      ;; Draft with specs ready to commit
      ((draft-ready-p drafts)
       (let ((name (ship-name (first drafts))))
         (format t "[LISP] -> bc ~A~%" name)
         (list (make-cmd "bc" name))))

      ;; Draft needs specs
      ((draft-needs-specs-p drafts)
       (let ((name (ship-name (first drafts))))
         (format t "[LISP] -> bs ~A~%" name)
         (list (make-cmd "bs" (format nil "~A PD=5 B=3 S=2 T=1 M=3" name)))))

      ;; Ship needs deployment
      ((ship-needs-deploy-p ships)
       (let ((ship (find-undeployed-ship ships)))
         (format t "[LISP] -> ds ~A~%" (ship-name ship))
         (list (make-cmd "ds" (format nil "~A ASTREX" (ship-name ship))))))

      ;; Should we start building a new ship?
      ((should-build-p slate)
       (let ((name (next-ship-name)))
         (format t "[LISP] -> bn ~A~%" name)
         (list (make-cmd "bn" (format nil "W ~A" name)))))

      ;; Done with build phase
      (t
       (format t "[LISP] -> NEXT~%")
       (list (cmd-next))))))

;;; ----------------------------------------------------------------------------
;;; State Predicates
;;; ----------------------------------------------------------------------------

(defun draft-ready-p (drafts)
  "Check if first draft has specs and is ready to commit."
  (and drafts
       (let ((d (first drafts)))
         (> (ship-pd d) 0))))

(defun draft-needs-specs-p (drafts)
  "Check if first draft exists but has no specs."
  (and drafts
       (let ((d (first drafts)))
         (<= (ship-pd d) 0))))

(defun ship-needs-deploy-p (ships)
  "Check if any ship needs deployment."
  (find-undeployed-ship ships))

(defun find-undeployed-ship (ships)
  "Find a ship with no hex (not deployed)."
  (find-if (lambda (s)
             (let ((hex (ship-hex s)))
               (or (null hex) (string= hex ""))))
           ships))
