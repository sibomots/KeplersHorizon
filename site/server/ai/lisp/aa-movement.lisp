;;;; aa-movement.lisp - Movement Phase Decision Logic

;;; ----------------------------------------------------------------------------
;;; Movement Phase Entry
;;; ----------------------------------------------------------------------------

(defun decide-movement-phase (slate)
  "Decide what to do in movement phase.
   Uses C++ computed suggested destinations for pathfinding."
  (let ((ship (find-ship-with-move slate)))
    (format t "[LISP] decide-movement: looking for ships with suggested moves~%")
    (if ship
        (let ((dest (ship-suggested-dest ship)))
          (format t "[LISP] -> move ~A to ~A~%" (ship-name ship) dest)
          (list (make-cmd "m" (format nil "~A ~A" (ship-name ship) dest))))
        (progn
          (format t "[LISP] -> advance (no suggested moves)~%")
          (list (cmd-next))))))

;;; ----------------------------------------------------------------------------
;;; Movement Helpers
;;; ----------------------------------------------------------------------------

(defun find-ship-with-move (slate)
  "Find a ship that has a C++ suggested destination."
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
