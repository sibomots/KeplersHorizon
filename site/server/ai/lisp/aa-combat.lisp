;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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
;;; Multi-Theater Triage
;;; ----------------------------------------------------------------------------
;;; Before issuing any combat order, assess ALL active theaters together.
;;; Decide where to fight vs retreat based on:
;;;   - Force ratio (can we win?)
;;;   - Hex VP value (is this an enemy base?)
;;;   - Ship tech value (are our ships here worth preserving?)

(defun triage-all-theaters (slate &optional strategy)
  "Assess all combat theaters and return triage decisions.
   Returns alist of (hex . :fight/:retreat/:hold) for each combat hex.
   Call this ONCE before issuing any combat orders."
  (let* ((combats (slate-active-combats slate))
         (own-ships (slate-own-ships slate))
         (enemy-ships (slate-enemy-ships slate))
         (enemy-bases (slate-enemy-bases slate))
         (risk-tolerance (if strategy
                            (or (getf strategy :risk-tolerance) :normal)
                            :normal))
         (raw-acceptable (if strategy
                             (or (getf strategy :acceptable-loss-threshold) 0)
                             0))
         ;; Risk-tolerance adjusts acceptable-loss
         (acceptable-loss (case risk-tolerance
                            (:high (max raw-acceptable 1))
                            (:low 0)
                            (t raw-acceptable)))
         (results nil))
    (dolist (combat combats)
      (let* ((hex (combat-hex combat))
             (our-ships-here (ships-at-hex own-ships hex))
             (their-ships-here (ships-at-hex enemy-ships hex))
             (assessment (assess-theater hex our-ships-here their-ships-here
                                         enemy-bases acceptable-loss)))
        (push (cons hex assessment) results)))
    (format t "[LISP] Theater triage: ~A~%" results)
    (nreverse results)))

(defun assess-theater (hex our-ships their-ships enemy-bases
                       &optional (acceptable-loss 0))
  "Assess a single theater. Returns :fight, :retreat, or :hold.
   Uses combat-power weighting (PD + beam*1.5 + screen + tech*2) and
   screen penetration check for smarter triage.
   ACCEPTABLE-LOSS: 0=none, 1=one ship, 2=expendable ships OK to sacrifice."
  (let* ((our-power (reduce #'+ (mapcar #'ship-combat-power our-ships)
                            :initial-value 0.0))
         (their-power (reduce #'+ (mapcar #'ship-combat-power their-ships)
                              :initial-value 0.0))
         (our-pd (reduce #'+ (mapcar #'ship-pd our-ships) :initial-value 0))
         (their-pd (reduce #'+ (mapcar #'ship-pd their-ships) :initial-value 0))
         (is-vp-hex (member hex enemy-bases :test #'string=))
         (force-ratio (if (> their-power 0.0)
                          (/ our-power their-power)
                          99.0))
         (have-warpship (some #'ship-warpship-p our-ships))
         (can-penetrate (can-guarantee-damage-p our-ships their-ships)))
    (format t "[LISP] Theater ~A: our-power=~,1F their-power=~,1F ratio=~,2F vp=~A penetrate=~A loss-ok=~A~%"
            hex our-power their-power force-ratio is-vp-hex can-penetrate acceptable-loss)
    (cond
      ;; VP hex: fight unless badly outmatched
      ((and is-vp-hex (>= force-ratio 0.5))
       :fight)
      ;; VP hex but badly outmatched: hold (dodge) to buy time
      ((and is-vp-hex (< force-ratio 0.5))
       :hold)
      ;; Non-VP hex with advantage and can penetrate: fight
      ((and (>= force-ratio 1.2) can-penetrate)
       :fight)
      ;; Acceptable loss: fight at low-value hex even when outmatched
      ((and (> acceptable-loss 0) (not is-vp-hex) (>= force-ratio 0.4))
       (format t "[LISP] Acceptable loss threshold allows fighting~%")
       :fight)
      ;; Non-VP hex with advantage but can't penetrate: hold for reinforcements
      ((and (>= force-ratio 1.2) (not can-penetrate))
       (format t "[LISP] Can't penetrate screens despite advantage - holding~%")
       :hold)
      ;; Non-VP hex, matched: hold and evaluate
      ((>= force-ratio 0.7)
       :hold)
      ;; Non-VP hex, outmatched: retreat if possible
      ((and have-warpship (< force-ratio 0.7))
       :retreat)
      ;; Systemship can't retreat: hold
      (t :hold))))

(defun get-theater-triage (triage-results hex)
  "Look up triage decision for a hex. Default to :fight if not found."
  (let ((entry (assoc hex triage-results :test #'string=)))
    (if entry (cdr entry) :fight)))

;;; ----------------------------------------------------------------------------
;;; Combat Phase Entry
;;; ----------------------------------------------------------------------------

(defun decide-combat-phase (slate &optional strategy)
  "Decide ONE combat action. Called repeatedly until NEXT.
   RULE: Focus on ONE combat hex at a time. Finish it before moving to next.
   STRATEGY: full strategic state plist (used for tech-weighted triage).
   Priority:
   1. Ships with escape_pending need retreat command
   2. Ships with pending_damage need damage assignment
   3. Ships needing orders in actionable combat hex get combat orders
   4. If all orders issued but not committed, commit
   5. If all combats waiting for enemy/user, do nothing (wait)
   6. Otherwise advance"
  (let* ((combats (slate-active-combats slate))
         (own-ships (slate-own-ships slate))
         ;; THEATER TRIAGE: Assess all combats before acting
         (triage (when combats (triage-all-theaters slate strategy)))
         ;; Find combat that needs AI action (stage 0 with ships needing orders)
         (focus-combat (find-actionable-combat combats own-ships))
         (focus-hex (when focus-combat (combat-hex focus-combat)))
         ;; Filter ships to only those in focus hex
         (ships-in-focus (when focus-hex
                           (remove-if-not
                            (lambda (s) (string= (ship-hex s) focus-hex))
                            own-ships)))
         ;; Get triage decision for focus hex
         (theater-decision (when focus-hex
                             (get-theater-triage triage focus-hex))))
    (format t "[LISP] decide-combat: combats=~A focus-hex=~A ships-in-focus=~A triage=~A~%"
            (length combats) focus-hex (length ships-in-focus) theater-decision)
    (cond
      ;; Priority 1: Handle escaping ships (any hex)
      ((find-escaping-ship own-ships)
       (let ((ship (find-escaping-ship own-ships)))
         (format t "[LISP] -> retreat ~A~%" (ship-name ship))
         (issue-retreat ship slate strategy)))

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
         (format t "[LISP] -> combat order for ~A in ~A (focus: ~A stalemate: ~A triage: ~A)~%"
                 (ship-name ship) hex
                 (when focus-target (ship-code focus-target)) stalemate theater-decision)
         (issue-combat-order-with-triage ship focus-target enemies-here
                                         stalemate ai-attacker theater-decision
                                         strategy)))

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
                        own-ships))
           (committed (combat-ai-committed-p ch))
           (needs-order (when ships-here
                          (some #'ship-needs-order-p ships-here))))
      (format t "[LISP] find-actionable: hex=~A stage=~A ships=~A committed=~A needs-order=~A~%"
              hex stage (length ships-here) committed needs-order)
      (when ships-here
        (dolist (s ships-here)
          (format t "[LISP]   ship ~A hex=~A needs-order=~A~%"
                  (ship-code s) (ship-hex s) (ship-needs-order-p s))))
      ;; Only return this combat if AI has action to take
      (cond
        ;; Stage 0: need to issue orders or commit
        ((= stage 0)
         (cond
           ;; Need to commit (have ships, none need orders, not committed)
           ((and ships-here
                 (not committed)
                 (not needs-order))
            ch)
           ;; Need to issue orders
           ((and ships-here
                 (not committed)
                 needs-order)
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

(defun issue-retreat (ship &optional slate strategy)
  "Issue retreat command. Prefer strategic redeployment targets
   (defense/attack assignments) over simple nearest-base."
  ;; Format: retreat SHIPCODE hXXYY
  (let* ((hex (ship-hex ship))
         (own-bases (when slate (slate-own-bases slate)))
         ;; Check if strategy has a better retreat direction
         (strategic-target (find-strategic-retreat-target ship strategy))
         (adj-hex (ensure-hex-prefix
                   (cond
                     (strategic-target
                      (compute-retreat-toward-base hex (list strategic-target)
                                                  slate))
                     (own-bases
                      (compute-retreat-toward-base hex own-bases slate))
                     (t
                      (compute-retreat-hex hex))))))
    (list (make-cmd "retreat" (format nil "~A ~A" (ship-code ship) adj-hex)))))

(defun find-strategic-retreat-target (ship strategy)
  "Find a strategic hex to retreat toward (reinforce another theater).
   Checks reinforcement-availability first for theaters needing this ship,
   then falls back to defense/attack assignments.
   Returns hex string or NIL."
  (when strategy
    (let ((code (ship-code ship))
          (reinforcements (getf strategy :reinforcement-availability))
          (theater-priorities (getf strategy :theater-priorities))
          (defense-assigns (getf strategy :defense-assignments))
          (attack-assigns (getf strategy :attack-assignments)))
      ;; Priority 1: Check if this ship is listed as a reinforcement for
      ;; the highest-priority theater
      (when (and reinforcements theater-priorities)
        (dolist (tp theater-priorities)
          (let* ((theater-hex (car tp))
                 (avail (cdr (assoc theater-hex reinforcements
                                    :test #'string-equal))))
            (when (assoc code avail :test #'string-equal)
              (return-from find-strategic-retreat-target theater-hex)))))
      ;; Priority 2: Defense assignments needing reinforcement
      (dolist (assign defense-assigns)
        (when (not (string= (car assign) code))
          (return-from find-strategic-retreat-target (cdr assign))))
      ;; Priority 3: Attack assignments
      (dolist (assign attack-assigns)
        (when (not (string= (car assign) code))
          (return-from find-strategic-retreat-target (cdr assign))))))
  nil)

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
             ;; BIGBUG: Should pass home-side through slate
             ;; BIGBUG: We should NOT assume either side is belongs to
             ;;         a player until they CLAIM IT by deploying their
             ;;         ship to a known SIDE the first time.
             (new-col (min 24 (1+ col))))  ; Retreat toward right (side B)
        (format nil "~2,'0D~2,'0D" row new-col))
      "0000"))

(defun compute-retreat-toward-base (hex bases &optional slate)
  "Compute retreat hex toward nearest owned base.
   Uses BFS distance from slate when available, falls back to Manhattan."
  (if (null bases)
      (compute-retreat-hex hex)
      (let* ((nearest (first (sort (copy-list bases)
                                   (lambda (a b)
                                     (if slate
                                         (< (slate-distance slate hex a)
                                            (slate-distance slate hex b))
                                         (< (hex-manhattan-dist hex a)
                                            (hex-manhattan-dist hex b)))))))
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
  (issue-combat-order-with-triage ship target enemies 0 nil :fight))

(defun issue-combat-order-with-triage (ship target enemies stalemate ai-attacker
                                      triage-decision &optional strategy)
  "Issue combat order considering theater triage, stalemate state, and endgame.
   TRIAGE-DECISION: :fight, :retreat, or :hold (from theater triage)
   STRATEGY: full strategic state plist (for endgame overrides)
   Stalemate rules still apply: if AI attacker and stalemate>=2, must deal damage."
  (let* ((endgame-p (when strategy (getf strategy :endgame-p)))
         (vp-winner (when strategy (getf strategy :vp-race-winner)))
         (hex (ship-hex ship))
         (enemy-bases (when strategy (getf strategy :bases-held)))
         (at-held-base (when enemy-bases
                         (member hex enemy-bases :test #'string-equal)))
         (analysis
          (cond
            ;; Endgame override: winning VP race at held enemy base → dodge-defend
            ((and endgame-p (eq vp-winner :us) at-held-base)
             (format t "[LISP] Endgame: winning VP race, defending held base~%")
             (crt-dodge-alloc (ship-pd ship) (ship-beam ship) (ship-screen ship)))
            ;; Endgame override: enemy winning VP race → force fight
            ((and endgame-p (eq vp-winner :them))
             (format t "[LISP] Endgame: enemy winning VP race, maximum aggression~%")
             (crt-attack-alloc (ship-pd ship) (ship-beam ship) (ship-screen ship)
                               (if target (or (ship-last-drive target) 0) 0)
                               :standard))
            ;; Auto-retreat: damaged warpship below threshold
            ((should-auto-retreat-p ship
               (if strategy (or (getf strategy :risk-tolerance) :normal) :normal))
             (format t "[LISP] Auto-retreat: ship ~A below HP threshold~%"
                     (ship-name ship))
             (list :tactic "e"
                   :alloc (list :d (ship-pd ship) :b 0 :s 0)
                   :missiles nil))
            ;; Stalemate danger overrides triage - must deal damage or retreat
            ((and ai-attacker (>= stalemate 2))
             (format t "[LISP] Stalemate override: must damage or retreat~%")
             (analyze-must-damage-situation ship target enemies))
            ;; Triage says retreat: escape if warpship
            ((and (eq triage-decision :retreat) (ship-warpship-p ship))
             (format t "[LISP] Triage: RETREAT - preserving ship for better fights~%")
             (list :tactic "e"
                   :alloc (list :d (ship-pd ship) :b 0 :s 0)
                   :missiles nil))
            ;; Triage says hold: dodge defensively
            ((eq triage-decision :hold)
             (format t "[LISP] Triage: HOLD - dodging defensively~%")
             (crt-dodge-alloc (ship-pd ship) (ship-beam ship) (ship-screen ship)))
            ;; Triage says fight: normal combat analysis
            (t
             (analyze-combat-situation ship target enemies)))))
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
   Check if we can actually penetrate before committing to attack."
  (format t "[LISP] STALEMATE DANGER: Must deal damage!~%")
  (let* ((my-pd (ship-pd ship))
         (my-beam (ship-beam ship))
         (my-tube (ship-tube ship))
         (my-missile (ship-missile ship))
         (my-tech (ship-tech ship))
         (enemy-drive (when target (or (ship-last-drive target) 0)))
         (can-pen (can-guarantee-damage-p (list ship) enemies)))
    ;; If we can't penetrate screens, retreat instead of wasting the round
    (when (and (not can-pen) (ship-warpship-p ship)
               (not (and (> my-tube 0) (> my-missile 0))))
      (format t "[LISP] Cannot penetrate screens, retreating instead~%")
      (return-from analyze-must-damage-situation
        (list :tactic "e"
              :alloc (list :d my-pd :b 0 :s 0)
              :missiles nil)))
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
  "Build the full combat order string including missiles.
   When firing missiles: T=<tube-count> followed by M=<drive> for each missile.
   The number of M= entries must equal the T= value."
  (let* ((tube-count (length missiles))
         (base (format nil "~A ~A ~A pd=~A b=~A s=~A"
                       (ship-code ship)
                       tactic
                       (ship-code target)
                       (getf alloc :d)
                       (getf alloc :b)
                       (getf alloc :s))))
    ;; Add tube count and missile drives if firing
    (if (> tube-count 0)
        (format nil "~A t=~A~{ m=~A~}" base tube-count missiles)
        base)))

(defun analyze-combat-situation (ship target enemies)
  "Analyze tactical situation and return (:tactic X :alloc (...) :missiles (...)).
   Uses CRT knowledge, revealed enemy orders, and ship role dispatch.
   Note: Beams/Screens and Missiles are mutually exclusive per round."
  (let* ((my-pd (ship-pd ship))
         (my-beam (ship-beam ship))
         (my-screen (ship-screen ship))
         (my-tube (ship-tube ship))
         (my-missile (ship-missile ship))
         (my-tech (ship-tech ship))
         (my-role (classify-ship-role ship))
         (total-enemy-pd (reduce #'+ (mapcar #'ship-pd enemies) :initial-value 0))
         ;; Check revealed enemy tactic from prior round
         (enemy-tactic (when target (ship-last-tactic target)))
         (enemy-drive (when target (ship-last-drive target)))
         (enemy-pd (when target (ship-pd target)))
         (enemy-screen (when target (ship-screen target)))
         (enemy-tech (when target (ship-tech target))))

    ;; Role-specific dispatch (Section 5: Ship Capability Matching)
    (case my-role
      ;; Missile boats: always fire missiles if available; retreat if empty
      (:missile-boat
       (cond
         ((and (> my-tube 0) (> my-missile 0))
          (format t "[LISP] Missile-boat ~A: firing missiles~%" (ship-name ship))
          (return-from analyze-combat-situation
            (crt-missile-alloc my-pd my-tube my-missile my-tech
                               (or enemy-drive 0))))
         ((ship-warpship-p ship)
          (format t "[LISP] Missile-boat ~A: out of missiles, retreating~%"
                  (ship-name ship))
          (return-from analyze-combat-situation
            (list :tactic "e"
                  :alloc (list :d my-pd :b 0 :s 0)
                  :missiles nil)))))

      ;; Fortress (systemship): always attack, can't retreat, max damage
      (:fortress
       (format t "[LISP] Fortress ~A: all-in attack~%" (ship-name ship))
       (return-from analyze-combat-situation
         (crt-attack-alloc my-pd my-beam my-screen (or enemy-drive 0) :standard))))

    ;; Standard analysis for brawler, interceptor, defender

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

;;; ----------------------------------------------------------------------------
;;; Ship-to-Target Matchup Scoring (Section 5)
;;; ----------------------------------------------------------------------------

(defun compute-matchup-score (ship target)
  "Expected damage exchange between SHIP and TARGET.
   Returns plist (:we-deal N :they-deal N :exchange-ratio N :favorable-p T/NIL).
   Based on beam+tech vs screen+tech for both sides."
  (let* ((our-damage (+ (ship-beam ship) (ship-tech ship)))
         (their-absorb (+ (ship-screen target) (ship-tech target)))
         (we-deal (max 0 (- our-damage their-absorb)))
         (their-damage (+ (ship-beam target) (ship-tech target)))
         (our-absorb (+ (ship-screen ship) (ship-tech ship)))
         (they-deal (max 0 (- their-damage our-absorb)))
         (exchange-ratio (if (> they-deal 0)
                             (/ (float we-deal) (float they-deal))
                             99.0)))
    (list :we-deal we-deal
          :they-deal they-deal
          :exchange-ratio exchange-ratio
          :favorable-p (> we-deal they-deal))))

;;; ----------------------------------------------------------------------------
;;; Auto-Retreat on Damage (Section 5)
;;; ----------------------------------------------------------------------------

(defun should-auto-retreat-p (ship &optional (risk-tolerance :normal))
  "T if ship HP is below retreat threshold.
   Thresholds by risk-tolerance: :low=60%, :normal=40%, :high=25%.
   Only applies to warpships (systemships can't retreat)."
  (when (ship-warpship-p ship)
    (let* ((current-hp (+ (ship-pd ship) (ship-beam ship)
                          (ship-screen ship) (ship-tube ship)))
           (max-hp (+ (ship-pd-max ship) (ship-beam-max ship)
                      (ship-screen-max ship) (ship-tube-max ship)))
           (health-pct (if (> max-hp 0)
                           (/ (float current-hp) (float max-hp))
                           1.0))
           (threshold (case risk-tolerance
                        (:low 0.60)
                        (:high 0.25)
                        (t 0.40))))
      (< health-pct threshold))))

(defun minimum-ships-to-penetrate (target-ship)
  "Calculate minimum brawlers needed to penetrate target's screens.
   Assumes brawler does ~5 damage (B=4 + T=1 avg tech)."
  (let* ((screen (ship-screen target-ship))
         (tech (ship-tech target-ship))
         (absorption (+ screen tech))
         (brawler-damage 5))  ; Conservative estimate
    (1+ (floor absorption brawler-damage))))
