;;;; aa-build.lisp - Build Phase Decision Logic

;;; Warpship costs:
;;;   WG = 5 BP (required for warpship)
;;;   PD, B, S, T, SR = 1 BP each
;;;   M = 3 missiles per 1 BP
;;;
;;; Minimum viable warpship: WG(5) + PD(1) = 6 BP (can move, nothing else)
;;; Basic fighter: WG(5) + PD(5) + B(3) + S(2) + T(1) + M(3) = 17 BP

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
  "Heuristic: Should we build another ship?
   - Keep a credit reserve for repairs/resupply
   - Build if we have fewer ships than enemy + 1
   - Don't overbuild in early game"
  (let ((credits (slate-credits slate))
        (our-ships (length (slate-own-ships slate)))
        (enemy-ships (length (slate-enemy-ships slate)))
        (round (slate-round slate)))
    ;; Must have enough credits beyond reserve
    (and (>= credits (+ *ship-cost* *credit-reserve*))
         ;; Fleet size heuristic: match enemy + small advantage
         ;; Early game (round 1): build up to 2 ships
         ;; Later: build to match enemy + 1, max 5
         (< our-ships (min 5 (max 2 (+ enemy-ships 1)))))))

(defun decide-build-phase (slate)
  "Decide what to do in build phase.
   Priority:
   1. If we have drafts, commit them
   2. If heuristics say build, build one
   3. Otherwise, advance"
  (let ((fleet-size (length (slate-own-ships slate)))
        (enemy-size (length (slate-enemy-ships slate)))
        (draft-count (length (slate-drafts slate))))
    (format t "[LISP] decide-build-phase: ships=~A enemy=~A drafts=~A credits=~A~%"
            fleet-size enemy-size draft-count (slate-credits slate))
    (cond
      ;; Commit pending drafts first
      ((has-drafts-p slate)
       (format t "[LISP] -> commit draft~%")
       (commit-first-draft slate))

      ;; Build if heuristic says so
      ((should-build-p slate)
       (format t "[LISP] -> build warpship~%")
       (build-basic-warpship slate))

      ;; Otherwise advance to movement
      (t
       (format t "[LISP] -> advance~%")
       (list (cmd-next))))))

;;; ----------------------------------------------------------------------------
;;; Build Actions
;;; ----------------------------------------------------------------------------

(defun commit-first-draft (slate)
  "Commit the first draft ship and deploy it."
  (let* ((draft (first (slate-drafts slate)))
         (name (ship-name draft)))
    ;; BC uses ship name, DS uses ship name + starbase name
    ;; AI player B's home starbase is ASTREX
    (list (make-cmd "bc" name)
          (make-cmd "ds" (format nil "~A ASTREX" name)))))

(defun build-basic-warpship (slate)
  "Build a basic warpship: PD=5 B=3 S=2 T=1 M=3 (17 BP total)."
  (declare (ignore slate))
  (let ((name (next-ship-name)))
    (list
     (make-cmd "bn" (format nil "W ~A" name))
     (make-cmd "bs" (format nil "~A PD=5 B=3 S=2 T=1 M=3" name))
     (make-cmd "bc" name))))

(defun build-scout-warpship (slate)
  "Build a minimal scout: PD=3 only (8 BP total)."
  (declare (ignore slate))
  (let ((name (next-ship-name)))
    (list
     (make-cmd "bn" (format nil "W ~A" name))
     (make-cmd "bs" (format nil "~A PD=3" name))
     (make-cmd "bc" name))))
