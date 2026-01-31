;;;; aa-combat.lisp - Combat Phase Decision Logic (CRT-Aware)

;;; Combat Stages:
;;;   0 = ORDERS      - Issue combat orders for each ship, then commit
;;;   1 = RESOLVE     - System resolves (automatic)
;;;   2 = DAMAGE      - Assign damage to ship attributes
;;;   3 = RETREAT     - Stalemate retreat (attacker must withdraw)

;;; CRT Summary (Drive Differential = my_drive - target_drive):
;;;   ATTACK vs ATTACK: best at -1 to +2 (Hit/Hit+2/Hit+1), misses at extremes
;;;   ATTACK vs DODGE:  only hits at +2 (Hit+1), +3/+4 (Hit)
;;;   ATTACK vs RETREAT: escapes at <=−1, hits at +3/+4
;;;   DODGE vs ATTACK:  hits at -2 to +2
;;;   DODGE vs DODGE:   hits at -2 to 0
;;;   DODGE vs RETREAT: always lets them escape
;;;   RETREAT vs any:   escapes most situations, hits attackers at -1/0 only

;;; ----------------------------------------------------------------------------
;;; Combat Phase Entry
;;; ----------------------------------------------------------------------------

(defun decide-combat-phase (slate)
  "Decide ONE combat action. Called repeatedly until NEXT.
   Priority:
   1. Ships with escape_pending need retreat command
   2. Ships with pending_damage need damage assignment
   3. Ships needing orders get combat orders (CRT-aware)
   4. If all orders issued but not committed, commit
   5. Otherwise advance"
  (let ((combats (slate-active-combats slate))
        (own-ships (slate-own-ships slate)))
    (format t "[LISP] decide-combat: combats=~A ships=~A~%"
            (length combats) (length own-ships))
    (cond
      ;; Priority 1: Handle escaping ships
      ((find-escaping-ship own-ships)
       (let ((ship (find-escaping-ship own-ships)))
         (format t "[LISP] -> retreat ~A~%" (ship-name ship))
         (issue-retreat ship)))

      ;; Priority 2: Assign pending damage
      ((find-ship-with-damage own-ships)
       (let ((ship (find-ship-with-damage own-ships)))
         (format t "[LISP] -> apply damage to ~A~%" (ship-name ship))
         (issue-damage-assignment ship)))

      ;; Priority 3: Issue combat orders for ships that need them
      ((find-ship-needing-order own-ships)
       (let ((ship (find-ship-needing-order own-ships)))
         (format t "[LISP] -> combat order for ~A~%" (ship-name ship))
         (issue-combat-order ship (slate-enemy-ships slate))))

      ;; Priority 4: Commit if orders are ready but not committed
      ((needs-combat-commit-p combats own-ships)
       (format t "[LISP] -> cc~%")
       (list (make-cmd "cc")))

      ;; Otherwise advance
      (t
       (format t "[LISP] -> NEXT (no combat action needed)~%")
       (list (cmd-next))))))

;;; ----------------------------------------------------------------------------
;;; Find Ships Needing Action
;;; ----------------------------------------------------------------------------

(defun find-escaping-ship (ships)
  "Find a ship with escape_pending."
  (find-if #'ship-escape-pending-p ships))

(defun find-ship-with-damage (ships)
  "Find a ship with pending damage to assign."
  (find-if (lambda (s) (> (ship-pending-damage s) 0)) ships))

(defun find-ship-needing-order (ships)
  "Find a ship that needs a combat order."
  (find-if #'ship-needs-order-p ships))

(defun needs-combat-commit-p (combats ships)
  "Check if we have orders to commit.
   True if: stage=0, AI has ships with orders, but not committed."
  (and combats
       (let ((ch (first combats)))
         (and (= (combat-stage ch) 0)
              (not (combat-ai-committed-p ch))
              ;; At least one ship is in combat (has hex matching combat)
              (some (lambda (s)
                      (string= (ship-hex s) (combat-hex ch)))
                    ships)
              ;; No ships still need orders
              (not (find-ship-needing-order ships))))))

;;; ----------------------------------------------------------------------------
;;; Issue Retreat
;;; ----------------------------------------------------------------------------

(defun issue-retreat (ship)
  "Issue retreat command. Pick adjacent hex toward friendly territory."
  ;; Format: retreat SHIPNAME hXXYY
  (let* ((hex (ship-hex ship))
         (adj-hex (compute-adjacent-hex hex)))
    (list (make-cmd "retreat" (format nil "~A h~A" (ship-name ship) adj-hex)))))

(defun compute-adjacent-hex (hex)
  "Compute an adjacent hex. Simple heuristic: decrement column (toward side A)."
  ;; Hex format is 4 digits: RRCC (row, col)
  ;; Decrement column to retreat toward left side of map
  (if (and hex (>= (length hex) 4))
      (let* ((row (parse-integer (subseq hex 0 2)))
             (col (parse-integer (subseq hex 2 4)))
             (new-col (max 1 (1- col))))
        (format nil "~2,'0D~2,'0D" row new-col))
      "0000"))

;;; ----------------------------------------------------------------------------
;;; Issue Damage Assignment
;;; ----------------------------------------------------------------------------

(defun issue-damage-assignment (ship)
  "Issue damage assignment command.
   Strategy: Preserve PD as long as possible (needed for maneuver/retreat)."
  (let* ((damage (ship-pending-damage ship))
         (pd (ship-pd ship))
         (beam (ship-beam ship))
         (screen (ship-screen ship))
         (tube (ship-tube ship))
         (total-hp (+ pd beam screen tube)))
    ;; If damage >= total HP, ship is destroyed - assign all
    (if (>= damage total-hp)
        (list (make-cmd "ca" (format nil "~A pd=~A b=~A s=~A t=~A"
                                     (ship-name ship) pd beam screen tube)))
        ;; Otherwise, allocate damage preserving PD
        (let ((alloc (allocate-damage damage beam screen tube pd)))
          (list (make-cmd "ca" (format nil "~A pd=~A b=~A s=~A t=~A"
                                       (ship-name ship)
                                       (getf alloc :pd)
                                       (getf alloc :b)
                                       (getf alloc :s)
                                       (getf alloc :t))))))))

(defun allocate-damage (damage beam screen tube pd)
  "Allocate damage to attributes. Preserve PD last (most valuable)."
  (let ((remaining damage)
        (d-beam 0)
        (d-screen 0)
        (d-tube 0)
        (d-pd 0))
    ;; First, take from tubes (least useful in beam combat)
    (when (> remaining 0)
      (let ((take (min remaining tube)))
        (incf d-tube take)
        (decf remaining take)))
    ;; Then screens (defensive, but PD is more versatile)
    (when (> remaining 0)
      (let ((take (min remaining screen)))
        (incf d-screen take)
        (decf remaining take)))
    ;; Then beams (offensive capability)
    (when (> remaining 0)
      (let ((take (min remaining beam)))
        (incf d-beam take)
        (decf remaining take)))
    ;; Finally PD (unavoidable, most critical)
    (when (> remaining 0)
      (let ((take (min remaining pd)))
        (incf d-pd take)
        (decf remaining take)))
    (list :pd d-pd :b d-beam :s d-screen :t d-tube)))

;;; ----------------------------------------------------------------------------
;;; Issue Combat Order (CRT-Aware)
;;; ----------------------------------------------------------------------------

(defun issue-combat-order (ship enemies)
  "Issue combat order for ship using CRT-aware tactics.
   Format: co SHIP TACTIC TARGET d=N b=N s=N"
  (let* ((hex (ship-hex ship))
         (enemies-here (ships-at-hex enemies hex))
         (target (pick-best-target ship enemies-here))
         (analysis (analyze-combat-situation ship target enemies-here))
         (tactic (getf analysis :tactic))
         (alloc (getf analysis :alloc)))
    (if target
        (progn
          (format t "[LISP] Combat: ~A vs ~A, tactic=~A, d=~A b=~A s=~A~%"
                  (ship-name ship) (ship-code target) tactic
                  (getf alloc :d) (getf alloc :b) (getf alloc :s))
          (list (make-cmd "co" (format nil "~A ~A ~A d=~A b=~A s=~A"
                                       (ship-name ship)
                                       tactic
                                       (ship-code target)
                                       (getf alloc :d)
                                       (getf alloc :b)
                                       (getf alloc :s)))))
        ;; No target visible - dodge defensively
        (list (make-cmd "co" (format nil "~A d W1 d=~A b=0 s=0"
                                     (ship-name ship)
                                     (ship-pd ship)))))))

(defun pick-best-target (ship enemies)
  "Pick the best target based on threat assessment.
   Prioritize: damaged enemies > low PD > high beam."
  (if (null enemies)
      nil
      (first (sort (copy-list enemies)
                   (lambda (a b)
                     ;; Lower PD = easier kill = higher priority
                     (< (ship-pd a) (ship-pd b)))))))

(defun analyze-combat-situation (ship target enemies)
  "Analyze tactical situation and return (:tactic X :alloc (...)).
   Uses CRT knowledge and revealed enemy orders."
  (let* ((my-pd (ship-pd ship))
         (my-beam (ship-beam ship))
         (my-screen (ship-screen ship))
         (enemy-count (length enemies))
         (total-enemy-pd (reduce #'+ (mapcar #'ship-pd enemies) :initial-value 0))
         ;; Check revealed enemy tactic from prior round
         (enemy-tactic (when target (ship-last-tactic target)))
         (enemy-drive (when target (ship-last-drive target))))

    (cond
      ;; Badly outmatched: retreat if warpship
      ((and (< my-pd (/ total-enemy-pd 2))
            (ship-warpship-p ship))
       (format t "[LISP] Outmatched (~A vs ~A total) - retreating~%"
               my-pd total-enemy-pd)
       (list :tactic "r" :alloc (list :d my-pd :b 0 :s 0)))

      ;; Enemy was retreating last round: ATTACK with high drive to catch (+3/+4)
      ((eql enemy-tactic #\R)
       (format t "[LISP] Enemy retreating - attacking to pursue~%")
       (crt-attack-alloc my-pd my-beam my-screen enemy-drive :pursue))

      ;; Enemy was dodging: ATTACK with drive to achieve +2 differential
      ((eql enemy-tactic #\D)
       (format t "[LISP] Enemy dodging - attacking with drive advantage~%")
       (crt-attack-alloc my-pd my-beam my-screen enemy-drive :counter-dodge))

      ;; Enemy was attacking: DODGE to make them miss, we can still hit
      ((eql enemy-tactic #\A)
       (format t "[LISP] Enemy attacking - dodging to counter~%")
       (crt-dodge-alloc my-pd my-beam my-screen))

      ;; No prior intel: use strength ratio
      ((> my-pd (* 1.5 (if target (ship-pd target) 1)))
       ;; Significant advantage: ATTACK aggressively
       (format t "[LISP] PD advantage - attacking~%")
       (crt-attack-alloc my-pd my-beam my-screen
                         (if target (ship-pd target) 3) :standard))

      ;; Roughly matched: DODGE for defense + opportunity
      (t
       (format t "[LISP] Matched strength - dodging~%")
       (crt-dodge-alloc my-pd my-beam my-screen)))))

;;; ----------------------------------------------------------------------------
;;; CRT-Based Power Allocation
;;; ----------------------------------------------------------------------------

(defun crt-attack-alloc (pd max-beam max-screen enemy-drive mode)
  "Allocate power for ATTACK tactic based on CRT.
   MODE: :standard (aim for +2), :pursue (aim for +3/+4), :counter-dodge (+2 to +4)"
  (let* ((target-diff (case mode
                        (:pursue 4)      ; Need +3/+4 to hit retreating
                        (:counter-dodge 3) ; Need +2 to +4 to hit dodging
                        (t 2)))          ; Standard: +2 is best vs attack
         ;; Drive needed to achieve target differential
         (drive-needed (+ enemy-drive target-diff))
         (drive-alloc (min pd drive-needed))
         ;; Remaining for offense
         (remaining (- pd drive-alloc))
         ;; Put rest into beam for damage
         (beam-alloc (min max-beam remaining)))
    (list :tactic "a"
          :alloc (list :d drive-alloc :b beam-alloc :s 0))))

(defun crt-dodge-alloc (pd max-beam max-screen)
  "Allocate power for DODGE tactic based on CRT.
   DODGE hits at close range (-2 to +2 vs attack, -2 to 0 vs dodge).
   Balance drive for defense, some beam for opportunistic hits, screen for absorb."
  (let* (;; Drive: ~40% for maneuver
         (drive-alloc (floor (* pd 0.4)))
         (remaining (- pd drive-alloc))
         ;; Screen: ~30% of remaining for damage absorption
         (screen-alloc (min max-screen (floor (* remaining 0.5))))
         (remaining2 (- remaining screen-alloc))
         ;; Beam: rest for opportunistic fire
         (beam-alloc (min max-beam remaining2)))
    (list :tactic "d"
          :alloc (list :d drive-alloc :b beam-alloc :s screen-alloc))))
