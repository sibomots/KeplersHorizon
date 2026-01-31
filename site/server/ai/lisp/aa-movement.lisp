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

(defun decide-movement-phase (slate)
  "Decide ONE movement action. Called repeatedly until NEXT.
   Strategic priorities:
   1. Defensive: Return ships to defend threatened bases
   2. Offensive: Spread ships to undefended enemy bases
   3. Support: Concentrate for combat if needed
   Returns NEXT when no beneficial moves remain."
  (let ((ship (find-ship-to-move slate)))
    (format t "[LISP] decide-movement: looking for movable ships~%")
    (if ship
        (let ((dest (ship-suggested-dest ship))
              (round (slate-round slate)))
          ;; Turn 1 restriction: can't move to enemy bases
          (if (and (= round 1) (is-enemy-base-p dest slate))
              (progn
                (format t "[LISP] Turn 1: can't move to enemy base, skip~%")
                (list (cmd-next)))
              (progn
                (format t "[LISP] -> move ~A to h~A~%" (ship-name ship) dest)
                (list (make-cmd "m" (format nil "~A h~A" (ship-code ship) dest))))))
        (progn
          (format t "[LISP] -> NEXT (no ships can move)~%")
          (list (cmd-next))))))

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
   Returns NIL if no defense needed."
  (let* ((own-bases (slate-own-bases slate))
         (enemy-ships (slate-enemy-ships slate))
         (own-ships (slate-own-ships slate))
         (threatened-bases (threatened-bases slate)))

    ;; If any base is threatened and undefended, return nearest ship
    (when threatened-bases
      (format t "[LISP] Bases under threat: ~A~%" threatened-bases)
      ;; Find a ship not at threatened base that could help
      ;; (For now, simplified - would need pathfinding to base)
      nil)))

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
                 (and ehex (hex-distance base ehex) (<= (hex-distance base ehex) 3))))
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
