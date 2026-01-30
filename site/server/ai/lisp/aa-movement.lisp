;;;; aa-movement.lisp - Movement Phase Decision Logic

;;; ----------------------------------------------------------------------------
;;; Movement Phase Entry
;;; ----------------------------------------------------------------------------

(defun decide-movement-phase (slate)
  "Decide what to do in movement phase.
   For now: just advance (movement AI is future work)."
  (declare (ignore slate))
  ;; STUB: Real movement logic would:
  ;; 1. Identify ships that can move
  ;; 2. Evaluate strategic positions (enemy bases, defense)
  ;; 3. Generate move commands
  (list (cmd-next)))

;;; ----------------------------------------------------------------------------
;;; Movement Helpers (future use)
;;; ----------------------------------------------------------------------------

(defun movable-ships (slate)
  "Get ships that can move (warpships with PD > 0)."
  (remove-if-not
   (lambda (s)
     (and (ship-warpship-p s)
          (> (ship-pd s) 0)))
   (slate-own-ships slate)))

(defun ship-can-reach-p (ship hex-from hex-to)
  "Check if ship can reach hex-to from hex-from.
   STUB: Need hex graph/warpline data."
  (declare (ignore ship hex-from hex-to))
  nil)
