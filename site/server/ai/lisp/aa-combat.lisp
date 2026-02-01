;;;; aa-combat.lisp - Combat Phase Decision Logic (CRT-Aware)

;;; Combat Stages:
;;;   0 = ORDERS      - Issue combat orders for each ship, then commit
;;;   1 = RESOLVE     - System resolves (automatic)
;;;   2 = DAMAGE      - Assign damage to ship attributes
;;;   3 = RETREAT     - Stalemate retreat (attacker must withdraw)

;;; CRT Summary (Drive Differential = my_drive - target_drive):
;;;   ATTACK vs ATTACK: best at -1 to +2 (Hit/Hit+2/Hit+1), MISSES at +5
;;;   ATTACK vs DODGE:  hits at +2 (Hit+1), +3/+4 (Hit), MISSES at +5
;;;   ATTACK vs RETREAT: hits at +3/+4, MISSES at +5
;;;   DODGE vs ATTACK:  hits at -2 to +2
;;;   DODGE vs DODGE:   hits at -2 to 0
;;;   DODGE vs RETREAT: always lets them escape
;;;   RETREAT vs any:   escapes most situations, hits attackers at -1/0 only

;;; CRITICAL: +5 or higher differential = MISS. Never over-allocate drive!

;;; ----------------------------------------------------------------------------
;;; Combat Phase Entry
;;; ----------------------------------------------------------------------------

(defun decide-combat-phase (slate)
  "Decide ONE combat action. Called repeatedly until NEXT.
   RULE: Focus on ONE combat hex at a time. Finish it before moving to next.
   Priority:
   1. Ships with escape_pending need retreat command
   2. Ships with pending_damage need damage assignment
   3. Ships needing orders in actionable combat hex get combat orders
   4. If all orders issued but not committed, commit
   5. If all combats waiting for enemy/user, do nothing (wait)
   6. Otherwise advance"
  (let* ((combats (slate-active-combats slate))
         (own-ships (slate-own-ships slate))
         ;; Find combat that needs AI action (stage 0 with ships needing orders)
         (focus-combat (find-actionable-combat combats own-ships))
         (focus-hex (when focus-combat (combat-hex focus-combat)))
         ;; Filter ships to only those in focus hex
         (ships-in-focus (when focus-hex
                           (remove-if-not
                            (lambda (s) (string= (ship-hex s) focus-hex))
                            own-ships))))
    (format t "[LISP] decide-combat: combats=~A focus-hex=~A ships-in-focus=~A~%"
            (length combats) focus-hex (length ships-in-focus))
    (cond
      ;; Priority 1: Handle escaping ships (any hex)
      ((find-escaping-ship own-ships)
       (let ((ship (find-escaping-ship own-ships)))
         (format t "[LISP] -> retreat ~A~%" (ship-name ship))
         (issue-retreat ship)))

      ;; Priority 2: Assign pending damage (any hex)
      ((find-ship-with-damage own-ships)
       (let ((ship (find-ship-with-damage own-ships)))
         (format t "[LISP] -> apply damage to ~A~%" (ship-name ship))
         (issue-damage-assignment ship)))

      ;; Priority 3: Issue combat orders for ships in FOCUS HEX only
      ((and focus-hex (find-ship-needing-order ships-in-focus))
       (let* ((ship (find-ship-needing-order ships-in-focus))
              (hex focus-hex)
              (enemies-here (ships-at-hex (slate-enemy-ships slate) hex))
              ;; FOCUS FIRE: All AI ships target the same enemy (weakest first)
              (focus-target (pick-focus-target enemies-here))
              (stalemate (combat-stalemate-count focus-combat))
              (ai-attacker (combat-ai-attacker-p focus-combat)))
         ;; Stalemate warning: if AI is attacker and stalemate=2, must deal damage
         (when (and ai-attacker (>= stalemate 2))
           (format t "[LISP] WARNING: Stalemate=~A, must deal damage or retreat!~%"
                   stalemate))
         (format t "[LISP] -> combat order for ~A in ~A (focus: ~A stalemate: ~A)~%"
                 (ship-name ship) hex
                 (when focus-target (ship-code focus-target)) stalemate)
         (issue-combat-order-with-stalemate ship focus-target enemies-here
                                            stalemate ai-attacker)))

      ;; Priority 4: Commit if orders are ready but not committed
      ((needs-combat-commit-p combats ships-in-focus focus-hex)
       (format t "[LISP] -> cc~%")
       (list (make-cmd "cc")))

      ;; Otherwise: check if we're waiting or can advance
      (t
       (if combats
           ;; Combats exist but no action for AI - waiting for enemy/user
           (progn
             (format t "[LISP] -> WAIT (combat active, awaiting enemy/user)~%")
             nil)
           ;; No combats - advance to next phase
           (progn
             (format t "[LISP] -> NEXT (no combat)~%")
             (list (cmd-next))))))))

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

(defun needs-combat-commit-p (combats ships-in-focus focus-hex)
  "Check if we have orders to commit for the focus hex.
   True if: stage=0, AI has ships with orders, but not committed."
  (and combats
       focus-hex
       ships-in-focus
       (let ((ch (find-combat-at-hex combats focus-hex)))
         (and ch
              (= (combat-stage ch) 0)
              (not (combat-ai-committed-p ch))
              ;; No ships in focus hex still need orders
              (not (find-ship-needing-order ships-in-focus))))))

(defun find-combat-at-hex (combats hex)
  "Find combat state for a specific hex."
  (find-if (lambda (ch) (string= (combat-hex ch) hex)) combats))

(defun find-actionable-combat (combats own-ships)
  "Return the FIRST combat if AI can act on it, NIL if waiting.
   RULE: Finish one hex completely before moving to the next.
   Combat ends only when: enemy destroyed OR stalemate retreat."
  (when combats
    (let* ((ch (first combats))
           (hex (combat-hex ch))
           (stage (combat-stage ch))
           (ships-here (remove-if-not
                        (lambda (s) (string= (ship-hex s) hex))
                        own-ships)))
      ;; Only return this combat if AI has action to take
      (cond
        ;; Stage 0: need to issue orders or commit
        ((= stage 0)
         (cond
           ;; Need to commit (have ships, none need orders, not committed)
           ((and ships-here
                 (not (combat-ai-committed-p ch))
                 (not (some #'ship-needs-order-p ships-here)))
            ch)
           ;; Need to issue orders
           ((and ships-here
                 (not (combat-ai-committed-p ch))
                 (some #'ship-needs-order-p ships-here))
            ch)
           ;; Waiting for enemy to commit - return NIL, don't skip to next hex
           (t nil)))
        ;; Stage 2: damage assignment
        ((= stage 2)
         (if (some (lambda (s) (> (ship-pending-damage s) 0)) ships-here)
             ch
             nil))
        ;; Stage 3: retreat
        ((= stage 3)
         (if (some #'ship-escape-pending-p ships-here)
             ch
             nil))
        ;; Stage 1 or other: resolution in progress, wait
        (t nil)))))

;;; ----------------------------------------------------------------------------
;;; Focus Fire - All AI ships target same enemy
;;; ----------------------------------------------------------------------------

(defun pick-focus-target (enemies)
  "Pick THE target for all AI ships. Prioritize by kill potential:
   1. Lowest total HP (PD+B+S+T) = easiest kill
   2. Lowest PD = least combat effective"
  (if (null enemies)
      nil
      (first (sort (copy-list enemies)
                   (lambda (a b)
                     (let ((hp-a (+ (ship-pd a) (ship-beam a)
                                    (ship-screen a) (ship-tube a)))
                           (hp-b (+ (ship-pd b) (ship-beam b)
                                    (ship-screen b) (ship-tube b))))
                       (< hp-a hp-b)))))))

;;; ----------------------------------------------------------------------------
;;; Issue Retreat
;;; ----------------------------------------------------------------------------

(defun issue-retreat (ship)
  "Issue retreat command. Pick adjacent hex toward friendly territory."
  ;; Format: retreat SHIPCODE hXXYY
  (let* ((hex (ship-hex ship))
         (adj-hex (compute-retreat-hex hex)))
    (list (make-cmd "retreat" (format nil "~A h~A" (ship-code ship) adj-hex)))))

(defun compute-retreat-hex (hex)
  "Compute retreat hex toward friendly bases.
   Uses home-side awareness if available, otherwise defaults."
  ;; Hex format is 4 digits: RRCC (row, col)
  (if (and hex (>= (length hex) 4))
      (let* ((row (parse-integer (subseq hex 0 2)))
             (col (parse-integer (subseq hex 2 4)))
             ;; Retreat toward column edge based on side
             ;; Side A bases are low columns, Side B are high columns
             ;; For now, assume AI is side B, retreat toward high columns
             ;; BUGBUG: Should pass home-side through slate
             (new-col (min 24 (1+ col))))  ; Retreat toward right (side B)
        (format nil "~2,'0D~2,'0D" row new-col))
      "0000"))

(defun compute-retreat-toward-base (hex bases)
  "Compute retreat hex toward nearest owned base."
  (if (null bases)
      (compute-retreat-hex hex)
      (let* ((nearest (first (sort (copy-list bases)
                                   (lambda (a b)
                                     (< (hex-manhattan-dist hex a)
                                        (hex-manhattan-dist hex b))))))
             (row (parse-integer (subseq hex 0 2)))
             (col (parse-integer (subseq hex 2 4)))
             (tgt-row (parse-integer (subseq nearest 0 2)))
             (tgt-col (parse-integer (subseq nearest 2 4)))
             (new-row (cond ((< row tgt-row) (1+ row))
                            ((> row tgt-row) (1- row))
                            (t row)))
             (new-col (cond ((< col tgt-col) (1+ col))
                            ((> col tgt-col) (1- col))
                            (t col))))
        (format nil "~2,'0D~2,'0D" new-row new-col))))

(defun hex-manhattan-dist (hex1 hex2)
  "Manhattan distance between two hexes."
  (if (and hex1 hex2 (>= (length hex1) 4) (>= (length hex2) 4))
      (let* ((r1 (parse-integer (subseq hex1 0 2)))
             (c1 (parse-integer (subseq hex1 2 4)))
             (r2 (parse-integer (subseq hex2 0 2)))
             (c2 (parse-integer (subseq hex2 2 4))))
        (+ (abs (- r1 r2)) (abs (- c1 c2))))
      999))

;;; ----------------------------------------------------------------------------
;;; Issue Damage Assignment
;;; ----------------------------------------------------------------------------

(defun issue-damage-assignment (ship)
  "Issue damage assignment command.
   Strategy: Preserve PD last (most valuable for retreat/combat).
   Secondary: Preserve screens (screen + tech = absorption).
   Uses base-pd (physical HP) not adjusted pd (power budget)."
  (let* ((damage (ship-pending-damage ship))
         (pd (ship-base-pd ship))
         (beam (ship-beam ship))
         (screen (ship-screen ship))
         (tube (ship-tube ship))
         (total-hp (+ pd beam screen tube)))
    ;; If damage >= total HP, ship is destroyed - assign all
    (if (>= damage total-hp)
        (list (make-cmd "ca" (build-ca-args (ship-code ship) pd beam screen tube)))
        ;; Otherwise, allocate damage preserving PD and screens
        (let ((alloc (allocate-damage damage tube beam screen pd)))
          (list (make-cmd "ca" (build-ca-args (ship-code ship)
                                              (getf alloc :pd)
                                              (getf alloc :b)
                                              (getf alloc :s)
                                              (getf alloc :t))))))))

(defun build-ca-args (ship-code pd beam screen tube)
  "Build CA command args, omitting zero values."
  (let ((parts (list ship-code)))
    (when (> pd 0)
      (push (format nil "pd=~A" pd) parts))
    (when (> beam 0)
      (push (format nil "b=~A" beam) parts))
    (when (> screen 0)
      (push (format nil "s=~A" screen) parts))
    (when (> tube 0)
      (push (format nil "t=~A" tube) parts))
    (format nil "~{~A~^ ~}" (nreverse parts))))

(defun allocate-damage (damage tube beam screen pd)
  "Allocate damage to attributes.
   Priority (sacrifice first): Tubes > Beams > Screens > PD
   Rationale: PD enables retreat, screens absorb (screen + tech)."
  (let ((remaining damage)
        (d-tube 0)
        (d-beam 0)
        (d-screen 0)
        (d-pd 0))
    ;; First, take from tubes (least useful)
    (when (> remaining 0)
      (let ((take (min remaining tube)))
        (incf d-tube take)
        (decf remaining take)))
    ;; Then beams (offensive)
    (when (> remaining 0)
      (let ((take (min remaining beam)))
        (incf d-beam take)
        (decf remaining take)))
    ;; Then screens (screen + tech = defensive value)
    (when (> remaining 0)
      (let ((take (min remaining screen)))
        (incf d-screen take)
        (decf remaining take)))
    ;; Finally PD (most critical - enables retreat)
    (when (> remaining 0)
      (let ((take (min remaining pd)))
        (incf d-pd take)
        (decf remaining take)))
    (list :pd d-pd :b d-beam :s d-screen :t d-tube)))

;;; ----------------------------------------------------------------------------
;;; Issue Combat Order (CRT-Aware)
;;; ----------------------------------------------------------------------------

(defun issue-combat-order (ship target enemies)
  "Issue combat order for ship using CRT-aware tactics.
   Format: co SHIP TACTIC TARGET d=N b=N s=N [m=N ...]
   Note: Beams/Screens cannot be used same round as missiles."
  (issue-combat-order-with-stalemate ship target enemies 0 nil))

(defun issue-combat-order-with-stalemate (ship target enemies stalemate ai-attacker)
  "Issue combat order considering stalemate state.
   If AI is attacker and stalemate >= 2, must guarantee damage or retreat."
  (let ((analysis (if (and ai-attacker (>= stalemate 2))
                      (analyze-must-damage-situation ship target enemies)
                      (analyze-combat-situation ship target enemies))))
    (if target
        (let* ((tactic (getf analysis :tactic))
               (alloc (getf analysis :alloc))
               (missiles (getf analysis :missiles))
               (cmd-str (build-combat-command ship target tactic alloc missiles)))
          (format t "[LISP] Combat: ~A vs ~A, tactic=~A, d=~A b=~A s=~A missiles=~A~%"
                  (ship-name ship) (ship-code target) tactic
                  (getf alloc :d) (getf alloc :b) (getf alloc :s)
                  (length missiles))
          (list (make-cmd "co" cmd-str)))
        ;; No target visible - dodge defensively
        (list (make-cmd "co" (format nil "~A d w1 pd=~A b=0 s=0"
                                     (ship-code ship)
                                     (ship-pd ship)))))))

(defun analyze-must-damage-situation (ship target enemies)
  "Stalemate danger: AI is attacker, 2 consecutive no-damage rounds.
   MUST deal damage this round or be forced to retreat next round.
   Be more aggressive - prioritize beams over screens."
  (format t "[LISP] STALEMATE DANGER: Must deal damage!~%")
  (let* ((my-pd (ship-pd ship))
         (my-beam (ship-beam ship))
         (my-tube (ship-tube ship))
         (my-missile (ship-missile ship))
         (my-tech (ship-tech ship))
         (enemy-drive (when target (or (ship-last-drive target) 0))))
    ;; If we have missiles, use them for guaranteed damage potential
    (if (and (> my-tube 0) (> my-missile 0))
        (crt-missile-alloc my-pd my-tube my-missile my-tech (or enemy-drive 0))
        ;; Otherwise, attack aggressively with max beam
        (let* ((beam-alloc my-beam)
               (drive-alloc (- my-pd beam-alloc)))
          (list :tactic "a"
                :alloc (list :d (max 1 drive-alloc) :b beam-alloc :s 0)
                :missiles nil)))))

(defun build-combat-command (ship target tactic alloc missiles)
  "Build the full combat order string including missiles."
  (let ((base (format nil "~A ~A ~A pd=~A b=~A s=~A"
                      (ship-code ship)
                      tactic
                      (ship-code target)
                      (getf alloc :d)
                      (getf alloc :b)
                      (getf alloc :s))))
    ;; Add missile specifications if any
    (if missiles
        (format nil "~A~{ m=~A~}" base missiles)
        base)))

(defun analyze-combat-situation (ship target enemies)
  "Analyze tactical situation and return (:tactic X :alloc (...) :missiles (...)).
   Uses CRT knowledge and revealed enemy orders.
   Note: Beams/Screens and Missiles are mutually exclusive per round."
  (let* ((my-pd (ship-pd ship))
         (my-beam (ship-beam ship))
         (my-screen (ship-screen ship))
         (my-tube (ship-tube ship))
         (my-missile (ship-missile ship))
         (my-tech (ship-tech ship))
         (total-enemy-pd (reduce #'+ (mapcar #'ship-pd enemies) :initial-value 0))
         ;; Check revealed enemy tactic from prior round
         (enemy-tactic (when target (ship-last-tactic target)))
         (enemy-drive (when target (ship-last-drive target)))
         (enemy-pd (when target (ship-pd target)))
         (enemy-screen (when target (ship-screen target)))
         (enemy-tech (when target (ship-tech target))))

    ;; Consider missile alpha strike if:
    ;; - Have tubes and missiles
    ;; - Enemy has high screens (beams won't penetrate well)
    ;; - First combat round (no revealed tactic = NIL)
    (when (and (> my-tube 0)
               (> my-missile 0)
               (null enemy-tactic)
               (> (+ (or enemy-screen 0) (or enemy-tech 0)) 3))
      (format t "[LISP] Missile alpha strike (enemy screens ~A+~A)~%"
              (or enemy-screen 0) (or enemy-tech 0))
      (return-from analyze-combat-situation
        (crt-missile-alloc my-pd my-tube my-missile my-tech (or enemy-drive 0))))

    (cond
      ;; Badly outmatched: retreat if warpship
      ((and (< my-pd (/ total-enemy-pd 2))
            (ship-warpship-p ship))
       (format t "[LISP] Outmatched (~A vs ~A total) - retreating~%"
               my-pd total-enemy-pd)
       (list :tactic "e" :alloc (list :d my-pd :b 0 :s 0) :missiles nil))

      ;; Enemy was retreating last round: ATTACK with +3/+4 drive to catch
      ((eql enemy-tactic #\E)
       (format t "[LISP] Enemy retreating - attacking to pursue~%")
       (crt-attack-alloc my-pd my-beam my-screen (or enemy-drive 0) :pursue))

      ;; Enemy was dodging: ATTACK with drive to achieve +3/+4 differential
      ((eql enemy-tactic #\D)
       (format t "[LISP] Enemy dodging - attacking with +3/+4 drive~%")
       (crt-attack-alloc my-pd my-beam my-screen (or enemy-drive 0) :counter-dodge))

      ;; Enemy was attacking: DODGE to make them miss at extremes
      ((eql enemy-tactic #\A)
       (format t "[LISP] Enemy attacking - dodging to counter~%")
       (crt-dodge-alloc my-pd my-beam my-screen))

      ;; No prior intel: use strength ratio
      ((> my-pd (* 1.5 (or enemy-pd 3)))
       ;; Significant advantage: ATTACK with optimal drive
       (format t "[LISP] PD advantage - attacking~%")
       (crt-attack-alloc my-pd my-beam my-screen (or enemy-pd 3) :standard))

      ;; Roughly matched: DODGE for defense + opportunity
      (t
       (format t "[LISP] Matched strength - dodging~%")
       (crt-dodge-alloc my-pd my-beam my-screen)))))

;;; ----------------------------------------------------------------------------
;;; CRT-Based Power Allocation
;;; ----------------------------------------------------------------------------

(defun crt-attack-alloc (pd max-beam max-screen enemy-drive mode)
  "Allocate power for ATTACK tactic based on CRT.
   CRITICAL: Cap drive differential at +4 (never +5 = miss)
   MODE: :standard (aim for +2), :pursue (aim for +4), :counter-dodge (+3/+4)"
  (let* ((target-diff (case mode
                        (:pursue 4)        ; Max to hit retreating
                        (:counter-dodge 3) ; Need +2 to +4 to hit dodging
                        (t 2)))            ; Standard: +2 is best vs attack
         ;; Drive needed to achieve target differential
         (drive-needed (+ enemy-drive target-diff))
         ;; CRITICAL: Never exceed +4 differential (clamp drive)
         (max-safe-drive (+ enemy-drive 4))
         (drive-alloc (min pd drive-needed max-safe-drive))
         ;; Remaining for offense
         (remaining (- pd drive-alloc))
         ;; Put rest into beam for damage
         (beam-alloc (min max-beam remaining)))
    (list :tactic "a"
          :alloc (list :d drive-alloc :b beam-alloc :s 0)
          :missiles nil)))

(defun crt-dodge-alloc (pd max-beam max-screen)
  "Allocate power for DODGE tactic based on CRT.
   DODGE hits at close range (-2 to +2 vs attack, -2 to 0 vs dodge).
   Balance drive for defense, some beam for opportunistic hits, screen for absorb."
  (let* (;; Drive: ~40% for maneuver
         (drive-alloc (floor (* pd 0.4)))
         (remaining (- pd drive-alloc))
         ;; Screen: prioritize absorption (screen + tech level)
         (screen-alloc (min max-screen (floor (* remaining 0.5))))
         (remaining2 (- remaining screen-alloc))
         ;; Beam: rest for opportunistic fire
         (beam-alloc (min max-beam remaining2)))
    (list :tactic "d"
          :alloc (list :d drive-alloc :b beam-alloc :s screen-alloc)
          :missiles nil)))

(defun crt-missile-alloc (pd tubes missiles tech enemy-drive)
  "Allocate for missile alpha strike.
   Missiles: independent drive up to PD + Tech.
   Each tube fires 1 missile per round, costs 1 PD to power.
   Note: Cannot use beams or screens when firing missiles."
  (let* (;; How many missiles can we fire?
         (max-missile-drive (+ pd tech))
         (fireable-count (min tubes missiles))
         ;; PD needed: 1 per tube powered
         (tube-pd fireable-count)
         ;; Remaining PD for drive (maneuver during combat)
         (drive-alloc (- pd tube-pd))
         ;; Set missile drive for optimal CRT hit vs dodgers (+3/+4)
         (missile-drive (min max-missile-drive (+ enemy-drive 4)))
         ;; Build list of missile drives
         (missile-drives (make-list fireable-count :initial-element missile-drive)))
    (format t "[LISP] Firing ~A missiles at drive ~A~%" fireable-count missile-drive)
    (list :tactic "a"  ; Missiles always attack
          :alloc (list :d drive-alloc :b 0 :s 0)
          :missiles missile-drives)))

;;; ----------------------------------------------------------------------------
;;; Force Concentration Analysis
;;; ----------------------------------------------------------------------------

(defun beam-damage (ship)
  "Calculate beam damage if hit: beam_power + tech_level."
  (+ (ship-beam ship) (ship-tech ship)))

(defun screen-absorption (ship screen-power)
  "Calculate screen absorption: screen_power + tech_level (if powered)."
  (if (> screen-power 0)
      (+ screen-power (ship-tech ship))
      0))

(defun calculate-force-balance (own-ships enemies)
  "Calculate if our force can penetrate enemy screens.
   Returns (:advantage T/NIL :damage-potential N :enemy-absorption N)"
  (let* (;; Our combined beam damage (assume all hit)
         (our-damage (reduce #'+ (mapcar #'beam-damage own-ships) :initial-value 0))
         ;; Enemy's screen absorption (assume powered)
         (enemy-absorb (reduce #'+
                               (mapcar (lambda (e)
                                         (+ (ship-screen e) (ship-tech e)))
                                       enemies)
                               :initial-value 0))
         ;; Net damage we can deal per round
         (net-damage (- our-damage enemy-absorb)))
    (list :advantage (> net-damage 0)
          :damage-potential our-damage
          :enemy-absorption enemy-absorb
          :net-damage net-damage)))

(defun can-guarantee-damage-p (own-ships enemies)
  "Check if our force concentration can guarantee damage through screens.
   This is crucial for avoiding stalemate traps."
  (let ((balance (calculate-force-balance own-ships enemies)))
    (when (getf balance :advantage)
      (format t "[LISP] Force balance: ~A damage vs ~A screens = ~A net~%"
              (getf balance :damage-potential)
              (getf balance :enemy-absorption)
              (getf balance :net-damage)))
    (getf balance :advantage)))

(defun minimum-ships-to-penetrate (target-ship)
  "Calculate minimum brawlers needed to penetrate target's screens.
   Assumes brawler does ~5 damage (B=4 + T=1 avg tech)."
  (let* ((screen (ship-screen target-ship))
         (tech (ship-tech target-ship))
         (absorption (+ screen tech))
         (brawler-damage 5))  ; Conservative estimate
    (1+ (floor absorption brawler-damage))))
