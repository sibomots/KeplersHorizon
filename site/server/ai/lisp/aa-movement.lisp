;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-movement.lisp - Movement Phase Decision Logic (Strategic)

;;; Movement Rules:
;;;   - Warpships move up to PD hexes per turn
;;;   - Must stop on star hexes containing enemies
;;;   - Cannot move to enemy base stars on turn 1
;;;   - Warplines: 1 PD to traverse entire line (efficient!)
;;;   - Pick up/drop systemship costs 1 PD each

;;; Strategic Priorities:
;;;   1. Defend own bases if threatened
;;;   2. Spread ships across enemy bases for VP
;;;   3. Concentrate force if combat needed
;;;   4. Use warplines for efficiency

;;; ----------------------------------------------------------------------------
;;; Movement Phase Entry
;;; ----------------------------------------------------------------------------

(defun decide-movement-phase (slate &optional strategy)
  "Decide ONE movement action. Called repeatedly until NEXT.
   Gen4: dispatches to movement rules via fire-first-producing-rule."
  (format t "[LISP] decide-movement: looking for movable ships~%")
  (fire-first-producing-rule :move slate strategy))

;;; ----------------------------------------------------------------------------
;;; Strategic Movement Selection
;;; ----------------------------------------------------------------------------

(defun find-ship-to-move (slate)
  "Find best ship to move based on strategic priority:
   1. Ships that can reach undefended enemy bases
   2. Ships with valid suggested destinations
   Skip ships already at enemy bases (they're scoring VP)."
  (let* ((own-ships (slate-own-ships slate))
         (enemy-bases (slate-enemy-bases slate))
         (own-bases (slate-own-bases slate))
         (round (slate-round slate)))

    ;; First check: any ship needs to defend home?
    (let ((defender (find-ship-for-defense slate)))
      (when defender
        (return-from find-ship-to-move defender)))

    ;; Otherwise find offensive move
    (find-if (lambda (s)
               (let ((dest (ship-suggested-dest s))
                     (hex (ship-hex s)))
                 (and dest
                      (not (string= dest ""))
                      ;; Not already at enemy base
                      (not (member hex enemy-bases :test #'string=))
                      ;; Has destination
                      (not (string= dest hex))
                      ;; Turn 1: can't go to enemy base
                      (or (> round 1)
                          (not (is-enemy-base-p dest slate))))))
             own-ships)))

(defun find-ship-for-defense (slate)
  "Find a ship to return home for defense if bases are threatened.
   Returns the ship with its suggested-dest overridden toward threatened base,
   or NIL if no defense needed."
  (let* ((own-bases (slate-own-bases slate))
         (enemy-ships (slate-enemy-ships slate))
         (own-ships (slate-own-ships slate))
         (enemy-bases (slate-enemy-bases slate))
         (threat-bases (threatened-bases slate)))

    (when threat-bases
      (format t "[LISP] Bases under threat: ~A~%" threat-bases)
      ;; Find unguarded threatened bases
      (dolist (tbase threat-bases)
        (let ((has-defender nil))
          (dolist (ship own-ships)
            (when (string= (ship-hex ship) tbase)
              (setf has-defender t)))
          (unless has-defender
            ;; Find nearest warpship not at an enemy base
            (let ((best-ship nil)
                  (best-dist 999))
              (dolist (ship own-ships)
                (when (and (ship-warpship-p ship)
                           (> (ship-pd ship) 0)
                           (not (member (ship-hex ship) enemy-bases
                                        :test #'string=)))
                  (let ((dist (hex-distance (ship-hex ship) tbase)))
                    (when (and dist (< dist best-dist) (> dist 0))
                      (setf best-dist dist)
                      (setf best-ship ship)))))
              (when best-ship
                (format t "[LISP] Assigning ~A to defend ~A~%"
                        (ship-name best-ship) tbase)
                (return-from find-ship-for-defense best-ship)))))))))

(defun threatened-bases (slate)
  "Return list of own bases that have enemy ships adjacent or approaching."
  (let* ((own-bases (slate-own-bases slate))
         (enemy-ships (slate-enemy-ships slate)))
    ;; Simplified: base is threatened if enemy ship is within 3 hexes
    ;; (Real implementation would use pathfinding)
    (remove-if-not
     (lambda (base)
       (some (lambda (enemy)
               (let ((ehex (ship-hex enemy)))
                 (and ehex (hex-distance base ehex)
                    (<= (hex-distance base ehex) (theta 'theta-movement-threat-range)))))
             enemy-ships))
     own-bases)))

(defun hex-distance (hex1 hex2)
  "Approximate hex distance (Manhattan-ish for hex grid).
   Hex format: RRCC (4 digits)."
  (if (and hex1 hex2 (>= (length hex1) 4) (>= (length hex2) 4))
      (let* ((r1 (parse-integer (subseq hex1 0 2)))
             (c1 (parse-integer (subseq hex1 2 4)))
             (r2 (parse-integer (subseq hex2 0 2)))
             (c2 (parse-integer (subseq hex2 2 4))))
        (+ (abs (- r1 r2)) (abs (- c1 c2))))
      999))

;;; ----------------------------------------------------------------------------
;;; Movement Helpers
;;; ----------------------------------------------------------------------------

(defun is-enemy-base-p (hex slate)
  "Check if hex is an enemy base star."
  (member hex (slate-enemy-bases slate) :test #'string=))

(defun is-own-base-p (hex slate)
  "Check if hex is our base star."
  (member hex (slate-own-bases slate) :test #'string=))

(defun movable-ships (slate)
  "Get ships that can move (warpships with PD > 0)."
  (remove-if-not
   (lambda (s)
     (and (ship-warpship-p s)
          (> (ship-pd s) 0)
          (let ((hex (ship-hex s)))
            (and hex (not (string= hex ""))))))
   (slate-own-ships slate)))

(defun ships-at-base (slate)
  "Get our ships currently at our bases."
  (let ((bases (slate-own-bases slate)))
    (remove-if-not
     (lambda (s)
       (member (ship-hex s) bases :test #'string=))
     (slate-own-ships slate))))

(defun ships-at-enemy-base (slate)
  "Get our ships currently at enemy bases (scoring VP)."
  (let ((enemy-bases (slate-enemy-bases slate)))
    (remove-if-not
     (lambda (s)
       (member (ship-hex s) enemy-bases :test #'string=))
     (slate-own-ships slate))))

(defun find-ship-by-code (ships code)
  "Find a ship in list by its code (case-insensitive)."
  (find-if (lambda (s) (string-equal (ship-code s) code)) ships))
