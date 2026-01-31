;;;; aa-core.lisp - AutonomyAgency Core Decision DSL
;;;;
;;;; Main entry point for ECL. C++ calls (aa-calculate slate) and
;;;; receives a list of command specs to render.
;;;;
;;;; NOTE: Sub-modules (aa-util, aa-build, aa-movement, aa-combat)
;;;; are loaded by C++ before this file. Do not add (load ...) here.

;;; Phase constants (must match C++ typedefs.h)
(defconstant +PH-BUILD-SHIPS+ 0)
(defconstant +PH-MOVEMENT+ 1)
(defconstant +PH-RESOLVE-COMBAT+ 2)
(defconstant +PH-SYSTEM-PICKDROP+ 3)
(defconstant +PH-END-TURN+ 4)

;;; ----------------------------------------------------------------------------
;;; Main Entry Point
;;; ----------------------------------------------------------------------------

(defun aa-calculate (slate)
  "Main decision function. Dispatches by phase.
   SLATE is an alist from C++.
   Returns list of command specs: ((:cmd \"CMD\" :args \"ARGS\") ...)"
  (format t "~&[LISP] aa-calculate called~%")
  (format t "[LISP] slate keys: ~A~%" (mapcar #'car slate))
  (let* ((phase-pair (assoc :phase slate))
         (phase (if phase-pair (cdr phase-pair) -1)))
    (format t "[LISP] phase-pair=~A phase=~A~%" phase-pair phase)
    (let ((result
            (case phase
              (#.+PH-BUILD-SHIPS+    (decide-build-phase slate))
              (#.+PH-MOVEMENT+       (decide-movement-phase slate))
              (#.+PH-RESOLVE-COMBAT+ (decide-combat-phase slate))
              (#.+PH-SYSTEM-PICKDROP+ (decide-pickdrop-phase slate))
              (#.+PH-END-TURN+       (decide-end-turn-phase slate))
              (otherwise             (list '(:cmd "NEXT" :args ""))))))
      (format t "[LISP] result=~A~%" result)
      result)))

;;; ----------------------------------------------------------------------------
;;; End Turn Phase
;;; ----------------------------------------------------------------------------

(defun decide-end-turn-phase (slate)
  "End of turn - just advance."
  (declare (ignore slate))
  (list '(:cmd "DONE" :args "")))

;;; ----------------------------------------------------------------------------
;;; Pick/Drop Phase (stub)
;;; ----------------------------------------------------------------------------

(defun decide-pickdrop-phase (slate)
  "SystemShip shuffling - advance for now."
  (declare (ignore slate))
  (list '(:cmd "NEXT" :args "")))
