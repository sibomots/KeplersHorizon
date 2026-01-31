;;;; aa-movement.lisp - Movement Phase Decision Logic (One Command at a Time)

;;; ----------------------------------------------------------------------------
;;; Movement Phase Entry
;;; ----------------------------------------------------------------------------

(defun decide-movement-phase (slate)
  "Decide ONE movement action. Called repeatedly until NEXT.
   Uses C++ computed suggested destinations for pathfinding.
   Returns NEXT when no ships have remaining PD to move."
  (let ((ship (find-ship-with-move slate)))
    (format t "[LISP] decide-movement: looking for movable ships~%")
    (if ship
        (let ((dest (ship-suggested-dest ship)))
          (format t "[LISP] -> move ~A to h~A~%" (ship-name ship) dest)
          (list (make-cmd "m" (format nil "~A h~A" (ship-name ship) dest))))
        (progn
          (format t "[LISP] -> NEXT (no ships can move)~%")
          (list (cmd-next))))))

;;; ----------------------------------------------------------------------------
;;; Movement Helpers
;;; ----------------------------------------------------------------------------

(defun find-ship-with-move (slate)
  "Find a ship that has a C++ suggested destination.
   C++ only suggests moves for ships with remaining PD > 0."
  (find-if (lambda (s)
             (let ((dest (ship-suggested-dest s)))
               (and dest (not (string= dest "")))))
           (slate-own-ships slate)))

(defun movable-ships (slate)
  "Get ships that can move (warpships with PD > 0)."
  (remove-if-not
   (lambda (s)
     (and (ship-warpship-p s)
          (> (ship-pd s) 0)))
   (slate-own-ships slate)))
