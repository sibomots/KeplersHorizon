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

(defun decide-build-phase (slate)
  "Decide what to do in build phase.
   Priority:
   1. If we have drafts, commit them
   2. If we can afford a warpship, build one
   3. Otherwise, advance"
  (format t "[LISP] decide-build-phase called~%")
  (format t "[LISP] credits=~A drafts=~A~%"
          (slate-credits slate) (slate-drafts slate))
  (cond
    ;; Commit pending drafts first
    ((has-drafts-p slate)
     (format t "[LISP] -> commit draft~%")
     (commit-first-draft slate))

    ;; Build a warpship if we can afford minimum (17 BP for a basic fighter)
    ((can-afford-p slate 17)
     (format t "[LISP] -> build warpship (credits=~A)~%" (slate-credits slate))
     (build-basic-warpship slate))

    ;; Otherwise advance to movement
    (t
     (format t "[LISP] -> advance (cannot afford)~%")
     (list (cmd-next)))))

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
