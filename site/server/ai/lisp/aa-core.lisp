;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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
  (reset-cycle-trace)
  (format t "~&[LISP] aa-calculate called~%")
  (format t "[LISP] slate keys: ~A~%" (mapcar #'car slate))
  (let* ((phase-pair (assoc :phase slate))
         (phase (if phase-pair (cdr phase-pair) -1))
         ;; Compute strategy once per phase call
         (strategy (handler-bind
                       ((error (lambda (c)
                                 (format t "[LISP] ERROR in compute-strategic-state: ~A~%" c)
                                 (format t "[LISP] BACKTRACE:~%")
                                 (si::tpl-backtrace))))
                     (compute-strategic-state slate))))
    (format t "[LISP] Posture: ~A | VDist: ~A | Ships: ~A/~A needed | Reason: ~A~%"
            (getf strategy :posture)
            (getf strategy :victory-distance)
            (getf strategy :warpship-count)
            (getf strategy :warpships-needed)
            (getf strategy :posture-reason))
    (format t "[LISP] phase-pair=~A phase=~A~%" phase-pair phase)
    (let ((result
            (case phase
              (#.+PH-BUILD-SHIPS+    (decide-build-phase slate strategy))
              (#.+PH-MOVEMENT+       (decide-movement-phase slate strategy))
              (#.+PH-RESOLVE-COMBAT+ (decide-combat-phase slate strategy))
              (#.+PH-SYSTEM-PICKDROP+ (decide-pickdrop-phase slate))
              (#.+PH-END-TURN+       (decide-end-turn-phase slate))
              (otherwise             (list '(:cmd "NEXT" :args ""))))))
      ;; Append metric write-backs (intercepted by C++, not sent to engine)
      (let ((metrics (compute-metrics-to-persist slate strategy)))
        (when metrics
          (setf result (append result metrics))))
      ;; Log rule trace for this cycle
      (let ((fired (fired-rules-this-cycle)))
        (when fired
          (format t "[LISP] Fired: ~{~A~^, ~}~%" fired)))
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
;;; Pick/Drop Phase
;;; ----------------------------------------------------------------------------

(defun decide-pickdrop-phase (slate)
  "Decide pick/drop actions for systemships.
   Gen4: dispatches to pickdrop rules via fire-first-matching-rule."
  (format t "[LISP] decide-pickdrop-phase~%")
  (fire-first-matching-rule :pickdrop slate nil))

(defun find-drop-opportunity (slate)
  "Find a warpship at enemy base with racked systemships to deploy.
   Returns a DROP command or NIL."
  (let ((own-ships (slate-own-ships slate))
        (enemy-bases (slate-enemy-bases slate)))
    (dolist (ship own-ships)
      (when (and (ship-warpship-p ship)
                 (> (ship-sr ship) 0))
        (let ((hex (ship-hex ship))
              (racked (ship-racked ship)))
          ;; Only drop at enemy bases (star systems)
          (when (and hex
                     (member hex enemy-bases :test #'string=)
                     racked
                     (> (length racked) 0))
            ;; Find first racked systemship to drop
            (let ((sysship-code (first racked)))
              (format t "[LISP] DROP opportunity: ~A from ~A at ~A~%"
                      sysship-code (ship-code ship) hex)
              (return-from find-drop-opportunity
                (make-cmd "drop" (format nil "~A ~A"
                                         (string-downcase sysship-code)
                                         (ship-code ship)))))))))
    nil))

(defun find-pick-opportunity (slate)
  "Find a free systemship to pick up with a warpship.
   Returns a PICK command or NIL."
  (let ((own-ships (slate-own-ships slate))
        (own-bases (slate-own-bases slate)))
    ;; Find warpships with available rack capacity
    (dolist (warp own-ships)
      (when (and (ship-warpship-p warp)
                 (> (ship-sr warp) 0))
        (let* ((hex (ship-hex warp))
               (racked (ship-racked warp))
               (racked-count (if racked (length racked) 0))
               (capacity (ship-sr warp)))
          ;; Only pick at star systems (check if at own base or any star)
          (when (and hex
                     (< racked-count capacity))
            ;; Find free systemships at same location
            (dolist (sys own-ships)
              (when (and (not (ship-warpship-p sys))
                         (not (ship-racked-p sys))
                         (string= (ship-hex sys) hex))
                (format t "[LISP] PICK opportunity: ~A onto ~A at ~A~%"
                        (ship-code sys) (ship-code warp) hex)
                (return-from find-pick-opportunity
                  (make-cmd "pick" (format nil "~A ~A"
                                           (ship-code sys)
                                           (ship-code warp))))))))))
    nil))
