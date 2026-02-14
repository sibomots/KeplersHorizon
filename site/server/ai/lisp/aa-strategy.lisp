;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-strategy.lisp - Strategic Ontology (3rd Generation)
;;;;
;;;; Full ontology: victory state, fleet assessment, ship assessments,
;;;; enemy profile, hex valuation, force projection, temporal state,
;;;; build directives, movement directives, posture.
;;;;
;;;; All functions are pure — slate in, plist out. No side effects.
;;;; Depends on: aa-util.lisp (slate accessors, ship accessors)

;;; ============================================================================
;;; A3 Helpers: Per-Ship Analysis
;;; ============================================================================

(defun ship-combat-power (ship)
  "Compute numeric combat power score for a single ship.
   Weights: PD + beam*1.5 + screen*1.2 + tech*2.5 + tube*0.5 + missile*0.3.
   Scaled by health-ratio (current stats / max stats) so damaged ships report lower."
  (let* ((raw (+ (ship-pd ship)
                 (* (ship-beam ship) 1.5)
                 (* (ship-screen ship) 1.2)
                 (* (ship-tech ship) 2.5)
                 (* (ship-tube ship) 0.5)
                 (* (ship-missile ship) 0.3)))
         (current-total (+ (ship-pd ship) (ship-beam ship)
                           (ship-screen ship) (ship-tube ship)))
         (max-total (+ (ship-pd-max ship) (ship-beam-max ship)
                       (ship-screen-max ship) (ship-tube-max ship)))
         (health-ratio (if (> max-total 0)
                           (/ (float current-total) (float max-total))
                           1.0)))
    (* raw health-ratio)))

(defun ship-combat-effective-p (ship)
  "T if ship has PD >= 50% of pd-max AND beam >= 50% of beam-max."
  (let ((pd (ship-pd ship))
        (pd-max (ship-pd-max ship))
        (beam (ship-beam ship))
        (beam-max (ship-beam-max ship)))
    (and (>= (* pd 2) pd-max)
         (>= (* beam 2) beam-max))))

(defun classify-ship-role (ship)
  "Classify ship into a role keyword based on stat thresholds.
   Returns :brawler, :interceptor, :missile-boat, :fortress, or :defender."
  (let ((pd (ship-pd ship))
        (beam (ship-beam ship))
        (tube (ship-tube ship))
        (warp (ship-warpship-p ship)))
    (cond
      ((> tube 0) :missile-boat)
      ((and (not warp) (>= pd 7)) :fortress)
      ((and (not warp)) :defender)
      ((and (>= pd 5) (>= beam 3)) :brawler)
      (t :interceptor))))

;;; ============================================================================
;;; A1: Victory & Score State
;;; ============================================================================

(defun compute-victory-state (slate)
  "Compute victory-related ontology facts from the slate.
   Returns plist with bases, distances, score state, VP rates, endgame,
   and risk tolerance."
  (let ((own-ships (slate-own-ships slate))
        (enemy-ships (slate-enemy-ships slate))
        (own-bases (slate-own-bases slate))
        (enemy-bases (slate-enemy-bases slate))
        (our-vp (slate-vp slate))
        (enemy-vp (slate-enemy-vp slate))
        (bases-held nil)
        (bases-needed nil)
        (own-at-risk nil)
        (vp-to-win 0)
        (victory-dist 0)
        (enemy-victory-dist 0)
        (urgent nil))

    ;; VP-to-win is total enemy base count
    (setf vp-to-win (length enemy-bases))

    ;; bases-held / bases-needed
    (dolist (ehex enemy-bases)
      (let ((held nil))
        (dolist (ship own-ships)
          (when (string= (ship-hex ship) ehex)
            (setf held t)))
        (if held
            (push ehex bases-held)
            (push ehex bases-needed))))
    (setf bases-held (nreverse bases-held))
    (setf bases-needed (nreverse bases-needed))

    ;; victory-distance
    (setf victory-dist (length bases-needed))

    ;; own-bases-at-risk
    (dolist (ohex own-bases)
      (dolist (eship enemy-ships)
        (when (string= (ship-hex eship) ohex)
          (pushnew ohex own-at-risk :test #'string=))))
    (setf own-at-risk (nreverse own-at-risk))

    ;; enemy-victory-distance
    (setf enemy-victory-dist (- (length own-bases) (length own-at-risk)))

    ;; urgent-p
    (setf urgent (or (= victory-dist 1)
                     (<= enemy-victory-dist 1)))

    ;; NEW: score state
    (let* ((score-state (cond ((> our-vp enemy-vp) :ahead)
                              ((< our-vp enemy-vp) :behind)
                              (t :tied)))
           (score-margin (abs (- our-vp enemy-vp)))
           (vp-rate (length bases-held))
           (enemy-vp-rate (length own-at-risk))
           (endgame-p (or (>= (+ our-vp 1) vp-to-win)
                          (>= (+ enemy-vp 1) (length own-bases))))
           (risk-tolerance (cond
                             ((eq score-state :behind) :high)
                             ((and (eq score-state :ahead) (>= score-margin 2)) :low)
                             (t :normal))))

      ;; VP projection: how many turns to win for each side
      (let* ((turns-to-win-us (if (> vp-rate 0)
                                  (ceiling (- vp-to-win our-vp) vp-rate)
                                  999))
             (turns-to-win-enemy (if (> enemy-vp-rate 0)
                                     (ceiling (- (length own-bases) enemy-vp)
                                              enemy-vp-rate)
                                     999))
             (vp-race-winner (cond
                               ((< turns-to-win-us turns-to-win-enemy) :us)
                               ((> turns-to-win-us turns-to-win-enemy) :them)
                               (t :tied))))
        (list :bases-held bases-held
              :bases-needed bases-needed
              :victory-distance victory-dist
              :own-bases-at-risk own-at-risk
              :enemy-victory-distance enemy-victory-dist
              :vp-to-win vp-to-win
              :urgent-p urgent
              :score-state score-state
              :score-margin score-margin
              :vp-rate vp-rate
              :enemy-vp-rate enemy-vp-rate
              :endgame-p endgame-p
              :risk-tolerance risk-tolerance
              :turns-to-win-us turns-to-win-us
              :turns-to-win-enemy turns-to-win-enemy
              :vp-race-winner vp-race-winner)))))

;;; ============================================================================
;;; A2: Fleet Assessment
;;; ============================================================================

(defun compute-fleet-state (slate)
  "Compute fleet-related ontology facts from the slate.
   Returns plist with ship counts, weapon totals, tech, concentration,
   and combat power comparisons."
  (let ((own-ships (slate-own-ships slate))
        (enemy-ships (slate-enemy-ships slate))
        (enemy-bases (slate-enemy-bases slate))
        (warp-count 0)
        (warp-at-bases nil)
        (warp-idle nil)
        (warp-needed 0)
        (fleet-pd 0)
        (enemy-pd 0)
        (enemy-warp-count 0)
        (fleet-beam 0)
        (fleet-screen 0)
        (fleet-tube 0)
        (fleet-missiles 0)
        (enemy-beam 0)
        (enemy-screen 0)
        (own-tech-sum 0)
        (enemy-tech-sum 0)
        (own-hex-set nil)
        (enemy-hex-set nil)
        (fleet-power 0.0)
        (enemy-power 0.0)
        (own-role-mix nil)
        (enemy-role-mix nil))

    ;; Own ship stats
    (dolist (ship own-ships)
      (incf fleet-pd (ship-pd ship))
      (incf fleet-beam (ship-beam ship))
      (incf fleet-screen (ship-screen ship))
      (incf fleet-tube (ship-tube ship))
      (incf fleet-missiles (ship-missile ship))
      (incf own-tech-sum (ship-tech ship))
      (incf fleet-power (ship-combat-power ship))
      (let ((hex (ship-hex ship)))
        (when (and hex (not (string= hex "")))
          (pushnew hex own-hex-set :test #'string=)))

      ;; Role counting
      (let ((role (classify-ship-role ship)))
        (let ((entry (assoc role own-role-mix)))
          (if entry
              (incf (cdr entry))
              (push (cons role 1) own-role-mix))))

      ;; Warpship classification
      (when (ship-warpship-p ship)
        (incf warp-count)
        (let ((hex (ship-hex ship))
              (on-enemy-base nil))
          (dolist (ehex enemy-bases)
            (when (string= hex ehex)
              (setf on-enemy-base t)))
          (if on-enemy-base
              (push ship warp-at-bases)
              (push ship warp-idle)))))

    (setf warp-at-bases (nreverse warp-at-bases))
    (setf warp-idle (nreverse warp-idle))

    ;; Enemy ship stats
    (dolist (eship enemy-ships)
      (incf enemy-pd (ship-pd eship))
      (incf enemy-beam (ship-beam eship))
      (incf enemy-screen (ship-screen eship))
      (incf enemy-tech-sum (ship-tech eship))
      (incf enemy-power (ship-combat-power eship))
      (when (ship-warpship-p eship)
        (incf enemy-warp-count))
      (let ((hex (ship-hex eship)))
        (when (and hex (not (string= hex "")))
          (pushnew hex enemy-hex-set :test #'string=)))
      ;; Enemy role counting
      (let ((role (classify-ship-role eship)))
        (let ((entry (assoc role enemy-role-mix)))
          (if entry
              (incf (cdr entry))
              (push (cons role 1) enemy-role-mix)))))

    ;; Warpships needed
    (setf warp-needed (max (length enemy-bases)
                           (1+ enemy-warp-count)))

    ;; Tech averages
    (let ((own-avg-tech (if own-ships
                            (/ (float own-tech-sum) (length own-ships))
                            0.0))
          (enemy-avg-tech (if enemy-ships
                              (/ (float enemy-tech-sum) (length enemy-ships))
                              0.0)))

      (list :warpship-count warp-count
            :warpships-at-bases warp-at-bases
            :warpships-idle warp-idle
            :warpships-needed warp-needed
            :fleet-total-pd fleet-pd
            :enemy-fleet-total-pd enemy-pd
            :fleet-advantage-p (> fleet-pd enemy-pd)
            :fleet-total-beam fleet-beam
            :fleet-total-screen fleet-screen
            :fleet-total-tube fleet-tube
            :fleet-missiles-loaded fleet-missiles
            :enemy-total-beam enemy-beam
            :enemy-total-screen enemy-screen
            :own-avg-tech own-avg-tech
            :enemy-avg-tech enemy-avg-tech
            :tech-advantage (- own-avg-tech enemy-avg-tech)
            :concentration-index (length own-hex-set)
            :enemy-concentration (length enemy-hex-set)
            :fleet-combat-power fleet-power
            :enemy-combat-power enemy-power
            :power-advantage-p (> fleet-power enemy-power)
            :own-role-mix own-role-mix
            :enemy-role-mix enemy-role-mix))))

;;; ============================================================================
;;; A3: Ship Assessments
;;; ============================================================================

(defun compute-ship-assessments (slate)
  "Per-ship analysis: combat readiness, damage, roles.
   Returns plist with combat-ready list, damaged list, effective count,
   and ship-roles alist."
  (let ((own-ships (slate-own-ships slate))
        (combat-ready nil)
        (damaged nil)
        (ship-roles nil))
    (dolist (ship own-ships)
      (if (ship-combat-effective-p ship)
          (push ship combat-ready)
          (push ship damaged))
      (push (cons (ship-code ship) (classify-ship-role ship)) ship-roles))
    (setf combat-ready (nreverse combat-ready))
    (setf damaged (nreverse damaged))
    (setf ship-roles (nreverse ship-roles))
    (list :combat-ready combat-ready
          :damaged-ships damaged
          :effective-count (length combat-ready)
          :ship-roles ship-roles)))

;;; ============================================================================
;;; A4: Enemy Profile
;;; ============================================================================

(defun compute-enemy-profile (slate)
  "Aggregate enemy intel: ship count, warpships, tech, weapons, positions,
   fleet composition by role, total combat power."
  (let ((enemy-ships (slate-enemy-ships slate))
        (own-bases (slate-own-bases slate))
        (enemy-count 0)
        (enemy-warp 0)
        (enemy-max-tech 0)
        (has-missiles nil)
        (high-screen nil)
        (positions nil)
        (at-our-bases nil)
        (enemy-fleet-power 0.0)
        (enemy-role-mix nil)
        (enemy-missile-capable 0))
    (dolist (eship enemy-ships)
      (incf enemy-count)
      (incf enemy-fleet-power (ship-combat-power eship))
      (when (ship-warpship-p eship)
        (incf enemy-warp))
      (when (> (ship-tech eship) enemy-max-tech)
        (setf enemy-max-tech (ship-tech eship)))
      (when (> (ship-tube eship) 0)
        (setf has-missiles t)
        (incf enemy-missile-capable))
      (when (>= (ship-screen eship) 3)
        (setf high-screen t))

      ;; Role counting
      (let ((role (classify-ship-role eship)))
        (let ((entry (assoc role enemy-role-mix)))
          (if entry
              (incf (cdr entry))
              (push (cons role 1) enemy-role-mix))))

      ;; Position tracking
      (let ((hex (ship-hex eship)))
        (when (and hex (not (string= hex "")))
          (let ((entry (assoc hex positions :test #'string=)))
            (if entry
                (incf (cdr entry))
                (push (cons hex 1) positions)))
          ;; At our bases?
          (when (member hex own-bases :test #'string=)
            (let ((base-entry (assoc hex at-our-bases :test #'string=)))
              (if base-entry
                  (push eship (cdr base-entry))
                  (push (cons hex (list eship)) at-our-bases)))))))

    (list :enemy-ship-count enemy-count
          :enemy-warpship-count enemy-warp
          :enemy-max-tech enemy-max-tech
          :enemy-has-missiles-p has-missiles
          :enemy-high-screen-p high-screen
          :enemy-positions (nreverse positions)
          :enemy-at-our-bases (nreverse at-our-bases)
          :enemy-fleet-power enemy-fleet-power
          :enemy-role-mix enemy-role-mix
          :enemy-missile-capable enemy-missile-capable)))

;;; ============================================================================
;;; A5: Hex Valuation
;;; ============================================================================

(defun compute-hex-value (slate hex &optional strategy)
  "Compute numeric composite score for any hex.
   Base scores, facility bonuses, proximity, warpline, resource value,
   chokepoint detection, denial value, and VP scoring rate."
  (let ((score 0)
        (enemy-bases (slate-enemy-bases slate))
        (own-bases (slate-own-bases slate))
        (facilities (slate-facilities slate)))
    ;; Base type bonuses
    (when (member hex enemy-bases :test #'string=)
      (incf score 100))
    (when (member hex own-bases :test #'string=)
      (incf score 80))

    ;; Facility bonuses
    (dolist (fac facilities)
      (when (string-equal (getf fac :system) hex)
        (let ((ftype (facility-type fac)))
          (cond
            ((string-equal ftype "SHIPYARD") (incf score 30))
            ((string-equal ftype "REFINERY") (incf score 20))
            ((string-equal ftype "REPAIR_DOCK") (incf score 15))
            ((string-equal ftype "TRADE_HUB") (incf score 10))))))

    ;; Proximity bonus: +10 for each base within BFS distance 4
    (dolist (base (append own-bases enemy-bases))
      (when (and (not (string= base hex))
                 (<= (slate-distance slate base hex) 4))
        (incf score 10)))

    ;; Warpline bonus: hex on a warpline has strategic value
    (when (warpline-hex-p slate hex)
      (incf score 15))

    ;; Resource value: hexes with rich extraction sites
    (let ((res-at-hex (resources-at-system slate hex)))
      (dolist (res res-at-hex)
        (let ((yield (resource-yield res)))
          (cond
            ((>= yield 8) (incf score 20))
            ((>= yield 4) (incf score 10))
            ((>= yield 2) (incf score 5))))))

    ;; Chokepoint detection: warpline hex connecting bases on both sides
    (when (warpline-hex-p slate hex)
      (let ((nbrs (hex-neighbors slate hex))
            (near-own 0)
            (near-enemy 0))
        (dolist (nbr nbrs)
          (when (member nbr own-bases :test #'string=)
            (incf near-own))
          (when (member nbr enemy-bases :test #'string=)
            (incf near-enemy)))
        (when (and (> near-own 0) (> near-enemy 0))
          (incf score 25))))

    ;; Denial value: hex on enemy threat path
    (when strategy
      (let ((threat-hexes (getf strategy :enemy-threat-hexes)))
        (when (and threat-hexes
                   (member hex threat-hexes :test #'string-equal))
          (incf score 10))))

    ;; VP scoring rate: enemy base we currently occupy is high-defend
    (when (member hex enemy-bases :test #'string=)
      (let ((occupied nil))
        (dolist (ship (slate-own-ships slate))
          (when (string= (ship-hex ship) hex)
            (setf occupied t)))
        (when occupied
          (incf score 40))))

    score))

(defun strip-hex-prefix (hex)
  "Remove leading 'h' from hex ID if present."
  (if (and hex (> (length hex) 0) (char= (char hex 0) #\h))
      (subseq hex 1)
      hex))

(defun compute-theater-map (slate)
  "Returns plist with valued-hexes, priority-target, defense-priority.
   Valued-hexes is sorted alist of (hex . score) for hexes with ships or bases."
  (let ((all-hexes nil)
        (own-ships (slate-own-ships slate))
        (enemy-ships (slate-enemy-ships slate))
        (own-bases (slate-own-bases slate))
        (enemy-bases (slate-enemy-bases slate))
        (hex-set nil))

    ;; Collect all relevant hexes
    (dolist (base own-bases)
      (pushnew base hex-set :test #'string=))
    (dolist (base enemy-bases)
      (pushnew base hex-set :test #'string=))
    (dolist (ship own-ships)
      (let ((hex (ship-hex ship)))
        (when (and hex (not (string= hex "")))
          (pushnew hex hex-set :test #'string=))))
    (dolist (ship enemy-ships)
      (let ((hex (ship-hex ship)))
        (when (and hex (not (string= hex "")))
          (pushnew hex hex-set :test #'string=))))

    ;; Score all hexes
    (dolist (hex hex-set)
      (push (cons hex (compute-hex-value slate hex)) all-hexes))

    ;; Sort by score descending
    (setf all-hexes (sort all-hexes (lambda (a b) (> (cdr a) (cdr b)))))

    ;; Priority target: highest-value unoccupied enemy base
    (let ((priority-target nil)
          (best-score -1))
      (dolist (ehex enemy-bases)
        (let ((occupied nil))
          (dolist (ship own-ships)
            (when (string= (ship-hex ship) ehex)
              (setf occupied t)))
          (when (not occupied)
            (let ((score (compute-hex-value slate ehex)))
              (when (> score best-score)
                (setf best-score score)
                (setf priority-target ehex))))))

      ;; Defense priority: highest-value own base at risk
      (let ((defense-priority nil)
            (best-def-score -1))
        (dolist (ohex own-bases)
          (let ((at-risk nil))
            (dolist (eship enemy-ships)
              (when (string= (ship-hex eship) ohex)
                (setf at-risk t)))
            (when at-risk
              (let ((score (compute-hex-value slate ohex)))
                (when (> score best-def-score)
                  (setf best-def-score score)
                  (setf defense-priority ohex))))))

        (list :valued-hexes all-hexes
              :priority-target priority-target
              :defense-priority defense-priority)))))

;;; ============================================================================
;;; A6: Force Projection
;;; ============================================================================

(defun compute-reachable-hexes (slate ship)
  "Compute set of hexes reachable by SHIP in one turn via BFS on adjacency data.
   Returns list of hex IDs within PD budget."
  (let ((pd (ship-pd ship))
        (start (ship-hex ship))
        (visited nil)
        (frontier nil))
    (when (and start (not (string= start "")) (> pd 0))
      (push (cons start 0) frontier)
      (loop while frontier do
        (let* ((current (pop frontier))
               (hex (car current))
               (cost (cdr current)))
          (unless (member hex visited :test #'string-equal)
            (push hex visited)
            (when (< cost pd)
              (dolist (nbr (hex-neighbors slate hex))
                (unless (member nbr visited :test #'string-equal)
                  (push (cons nbr (1+ cost)) frontier))))))))
    visited))

(defun compute-force-projection (slate)
  "Force deployment analysis: base classification, threat envelopes,
   coverage maps, convergence points, and reinforcement potential.
   Returns plist with all force projection data."
  (let ((own-ships (slate-own-ships slate))
        (enemy-ships (slate-enemy-ships slate))
        (own-bases (slate-own-bases slate))
        (enemy-bases (slate-enemy-bases slate))
        (undefended-enemy nil)
        (defended-enemy nil)
        (unguarded-own nil)
        (under-threat nil)
        (can-reinforce nil))

    ;; Classify enemy bases
    (dolist (ehex enemy-bases)
      (let ((has-enemy nil))
        (dolist (eship enemy-ships)
          (when (string= (ship-hex eship) ehex)
            (setf has-enemy t)))
        (if has-enemy
            (push ehex defended-enemy)
            (push ehex undefended-enemy))))
    (setf undefended-enemy (nreverse undefended-enemy))
    (setf defended-enemy (nreverse defended-enemy))

    ;; Classify own bases
    (dolist (ohex own-bases)
      (let ((has-friendly nil)
            (has-threat nil))
        (dolist (ship own-ships)
          (when (string= (ship-hex ship) ohex)
            (setf has-friendly t)))
        ;; Threat: enemy within BFS distance <= 6
        (dolist (eship enemy-ships)
          (let ((ehex (ship-hex eship)))
            (when (and ehex
                       (not (string= ehex ""))
                       (<= (slate-distance slate ehex ohex) 6))
              (setf has-threat t))))
        (when (not has-friendly)
          (push ohex unguarded-own))
        (when has-threat
          (push ohex under-threat))))
    (setf unguarded-own (nreverse unguarded-own))
    (setf under-threat (nreverse under-threat))

    ;; Can-reinforce: for each own base, list friendly ships within PD range
    (dolist (ohex own-bases)
      (let ((nearby nil))
        (dolist (ship own-ships)
          (when (ship-warpship-p ship)
            (let ((shex (ship-hex ship)))
              (when (and shex
                         (not (string= shex ""))
                         (not (string= shex ohex))
                         (<= (slate-distance slate shex ohex) (ship-pd ship)))
                (push ship nearby)))))
        (when nearby
          (push (cons ohex (nreverse nearby)) can-reinforce))))
    (setf can-reinforce (nreverse can-reinforce))

    ;; Threat envelopes and coverage maps (Section 2: Force Projection)
    (let ((enemy-threat-hexes nil)
          (friendly-coverage nil)
          (convergence-points nil)
          (friendly-reach-counts nil)
          (coverage-gaps nil))

      ;; Enemy threat map: union of all enemy ship reachable hexes
      (dolist (eship enemy-ships)
        (when (and (ship-warpship-p eship)
                   (> (ship-pd eship) 0))
          (let ((reachable (compute-reachable-hexes slate eship)))
            (dolist (hex reachable)
              (pushnew hex enemy-threat-hexes :test #'string-equal)))))

      ;; Friendly coverage map + convergence counting
      (dolist (ship own-ships)
        (when (and (ship-warpship-p ship)
                   (> (ship-pd ship) 0))
          (let ((reachable (compute-reachable-hexes slate ship)))
            (dolist (hex reachable)
              (pushnew hex friendly-coverage :test #'string-equal)
              (let ((entry (assoc hex friendly-reach-counts
                                  :test #'string-equal)))
                (if entry
                    (incf (cdr entry))
                    (push (cons hex 1) friendly-reach-counts)))))))

      ;; Convergence points: hexes reachable by 2+ friendly ships
      (dolist (entry friendly-reach-counts)
        (when (>= (cdr entry) 2)
          (push (car entry) convergence-points)))

      ;; Coverage gaps: own bases not in friendly coverage
      (dolist (base own-bases)
        (unless (member base friendly-coverage :test #'string-equal)
          (push base coverage-gaps)))

      (list :undefended-enemy-bases undefended-enemy
            :defended-enemy-bases defended-enemy
            :unguarded-own-bases unguarded-own
            :bases-under-threat under-threat
            :can-reinforce can-reinforce
            :enemy-threat-hexes enemy-threat-hexes
            :friendly-coverage friendly-coverage
            :convergence-points convergence-points
            :coverage-gaps coverage-gaps))))

;;; ============================================================================
;;; A6c: Theater Force Ratios (Section 4)
;;; ============================================================================

(defun compute-theater-force-ratios (slate)
  "Force ratio at each contested or relevant hex.
   Returns alist of (hex . (:our-power N :their-power N :ratio N))."
  (let ((own-ships (slate-own-ships slate))
        (enemy-ships (slate-enemy-ships slate))
        (contested (slate-contested-hexes slate))
        (own-bases (slate-own-bases slate))
        (enemy-bases (slate-enemy-bases slate))
        (hex-set nil)
        (results nil))
    ;; Collect all hexes with ships + bases
    (dolist (hex contested)
      (pushnew hex hex-set :test #'string-equal))
    (dolist (hex own-bases)
      (pushnew hex hex-set :test #'string-equal))
    (dolist (hex enemy-bases)
      (pushnew hex hex-set :test #'string-equal))
    (dolist (ship own-ships)
      (let ((hex (ship-hex ship)))
        (when (and hex (not (string= hex "")))
          (pushnew hex hex-set :test #'string-equal))))
    (dolist (ship enemy-ships)
      (let ((hex (ship-hex ship)))
        (when (and hex (not (string= hex "")))
          (pushnew hex hex-set :test #'string-equal))))
    ;; Compute force ratio at each hex
    (dolist (hex hex-set)
      (let ((our-power 0.0)
            (their-power 0.0))
        (dolist (ship own-ships)
          (when (string-equal (ship-hex ship) hex)
            (incf our-power (ship-combat-power ship))))
        (dolist (ship enemy-ships)
          (when (string-equal (ship-hex ship) hex)
            (incf their-power (ship-combat-power ship))))
        (when (or (> our-power 0) (> their-power 0))
          (let ((ratio (if (> their-power 0)
                           (/ our-power their-power)
                           99.0)))
            (push (cons hex (list :our-power our-power
                                  :their-power their-power
                                  :ratio ratio))
                  results)))))
    results))

;;; ============================================================================
;;; A6b: Enemy Intent (Section 9 - Opponent Modeling)
;;; ============================================================================

(defun compute-enemy-intent (slate)
  "Infer enemy intent from movement vectors, distance changes, and combat
   order watermarks. Returns plist with convergence, threat velocity, and
   enemy ship profiles."
  (let ((enemy-ships (slate-enemy-ships slate))
        (own-bases (slate-own-bases slate))
        (converge-counts nil)
        (threat-velocity nil)
        (ship-profiles nil))

    ;; Movement vector and convergence detection
    (dolist (eship enemy-ships)
      (let* ((code (getf eship :code))
             (hex (ship-hex eship))
             (prev-hex-metric
               (slate-metric slate
                             (format nil "enemy-~A-hex-prev" code) 0.0))
             (prev-hex-val (floor prev-hex-metric)))
        ;; Track where enemies are headed
        (when (and hex (not (string= hex ""))
                       (> prev-hex-val 0))
          ;; Check per-base distance changes
          (dolist (base own-bases)
            (let* ((cur-dist (slate-enemy-distance slate code base))
                   (prev-key (format nil "enemy-~A-dist-~A" code base))
                   (prev-dist (slate-metric slate prev-key 999.0)))
              (when (< cur-dist (floor prev-dist))
                ;; Enemy is closing on this base
                (let ((rate (- (floor prev-dist) cur-dist))
                      (existing (assoc base threat-velocity :test #'string=)))
                  (if existing
                      (incf (cdr existing) rate)
                      (push (cons base rate) threat-velocity)))))
            ;; Convergence: count enemies approaching same base
            (when (< (slate-enemy-distance slate code base) 6)
              (let ((existing (assoc base converge-counts :test #'string=)))
                (if existing
                    (incf (cdr existing))
                    (push (cons base 1) converge-counts))))))

        ;; Combat order watermarks (Category 4)
        (let* ((cur-pd (ship-last-drive eship))
               (cur-beam (ship-last-beam eship))
               (cur-screen (ship-last-screen eship))
               (cur-tube (ship-last-tube eship))
               (prev-max-pd
                 (slate-metric slate
                               (format nil "enemy-~A-max-pd-seen" code) 0.0))
               (prev-max-beam
                 (slate-metric slate
                               (format nil "enemy-~A-max-beam-seen" code) 0.0))
               (prev-max-screen
                 (slate-metric slate
                               (format nil "enemy-~A-max-screen-seen" code)
                               0.0))
               (prev-max-tube
                 (slate-metric slate
                               (format nil "enemy-~A-max-tube-seen" code) 0.0))
               (max-pd (max cur-pd (floor prev-max-pd)))
               (max-beam (max cur-beam (floor prev-max-beam)))
               (max-screen (max cur-screen (floor prev-max-screen)))
               (max-tube (max cur-tube (floor prev-max-tube))))
          (push (list code
                      :max-pd max-pd :max-beam max-beam
                      :max-screen max-screen :max-tube max-tube)
                ship-profiles))))

    ;; SystemRack inference (Category 5)
    ;; Detect pick/drop from enemy systemship position changes
    (let ((enemy-warpships
            (remove-if-not #'ship-warpship-p enemy-ships))
          (enemy-sysships
            (remove-if #'ship-warpship-p enemy-ships)))
      (dolist (sys enemy-sysships)
        (let* ((sys-code (getf sys :code))
               (sys-hex (ship-hex sys))
               (prev-metric
                 (slate-metric slate
                               (format nil "enemy-~A-hex-prev" sys-code) 0.0))
               (prev-val (floor prev-metric)))
          ;; Systemship vanished from a hex (prev-hex != 0 and cur-hex empty
          ;; or different)
          (when (and (> prev-val 0)
                     (or (string= sys-hex "")
                         (not (= prev-val
                                  (parse-integer
                                   (strip-hex-prefix sys-hex)
                                   :junk-allowed t)))))
            ;; Check if a warpship was co-located at prev hex
            (dolist (ws enemy-warpships)
              (let ((ws-code (getf ws :code))
                    (ws-prev
                      (floor
                       (slate-metric
                        slate
                        (format nil "enemy-~A-hex-prev" (getf ws :code))
                        0.0))))
                (when (= ws-prev prev-val)
                  ;; Warpship was at same hex when systemship vanished
                  ;; Mark as has-racks
                  (let ((profile (assoc ws-code ship-profiles
                                        :test #'string-equal)))
                    (when profile
                      (nconc profile
                             (list :has-racks t)))))))))))

    ;; Determine convergence target
    (let* ((sorted-converge
             (sort (copy-list converge-counts)
                   (lambda (a b) (> (cdr a) (cdr b)))))
           (converge-hex (when (and sorted-converge
                                    (>= (cdr (first sorted-converge)) 2))
                           (car (first sorted-converge))))
           (converge-count (if sorted-converge
                               (cdr (first sorted-converge))
                               0))
           ;; Enemy economic estimate (Section 9)
           ;; BP estimate: WG(5) + PD + B + S + T + ceil(M/3) + SR
           (enemy-total-bp 0))
      (dolist (eship enemy-ships)
        (let ((bp (+ (if (ship-warpship-p eship) 5 0)
                     (ship-pd eship) (ship-beam eship) (ship-screen eship)
                     (ship-tube eship) (ceiling (ship-missile eship) 3)
                     (ship-sr eship))))
          (incf enemy-total-bp bp)))
      (let* ((prev-bp-est (slate-metric slate "enemy-total-bp-est" 0.0))
             (enemy-spending-rate (cond
                                    ((> enemy-total-bp (floor prev-bp-est))
                                     :building)
                                    ((< enemy-total-bp (floor prev-bp-est))
                                     :lost-ships)
                                    (t :holding)))
             ;; Enemy strategy classification (requires visible enemies)
             (has-velocity (some (lambda (tv) (> (cdr tv) 0)) threat-velocity))
             (enemy-building-p (eq enemy-spending-rate :building))
             (enemy-saving-p (and (not enemy-building-p)
                                  (eq enemy-spending-rate :holding)
                                  (= converge-count 0)))
             (enemy-strategy (cond
                               ;; No enemies visible: unknown (skip classification)
                               ((null enemy-ships) :unknown)
                               ;; 3+ enemies converging on one base
                               ((>= converge-count 3) :aggressive)
                               ;; Enemies at bases AND closing
                               ((and (> converge-count 0) has-velocity) :aggressive)
                               ;; Saving and not building (only if enemies exist)
                               (enemy-saving-p :tech-investing)
                               ;; No threat velocity toward our bases
                               ((not has-velocity) :defensive)
                               ;; Default
                               (t :balanced))))
        (list :enemy-converging-on converge-hex
              :enemy-threat-velocity (nreverse threat-velocity)
              :enemy-ship-profiles (nreverse ship-profiles)
              :enemy-total-bp-est enemy-total-bp
              :enemy-spending-rate enemy-spending-rate
              :enemy-strategy enemy-strategy)))))

;;; ============================================================================
;;; A7: Temporal State
;;; ============================================================================

(defun compute-temporal-state (slate)
  "Time-aware facts: tech timing, game phase, cross-turn trends."
  (let* ((round (slate-round slate))
         (credits (slate-credits slate))
         (tech-boundary (* (1+ (floor (1- round) 4)) 4))
         (turns-until-tech (max 0 (- tech-boundary round)))
         (should-wait (and (= turns-until-tech 1)
                           (>= credits 15)))
         (game-phase (cond ((<= round 3) :early)
                           ((<= round 10) :mid)
                           (t :late)))
         ;; Trend analysis from persisted metrics
         (credits-prev (slate-metric slate "credits-prev" 0.0))
         (fleet-power-prev (slate-metric slate "fleet-power-prev" 0.0))
         (enemy-count-prev (slate-metric slate "enemy-count-prev" 0.0))
         (enemy-stable-rounds
           (slate-metric slate "enemy-count-stable-rounds" 0.0))
         ;; Compute current fleet power
         (own-ships (slate-own-ships slate))
         (fleet-power 0.0)
         (fleet-count (length own-ships))
         (enemy-count (length (slate-enemy-ships slate))))
    ;; Sum fleet power
    (dolist (ship own-ships)
      (incf fleet-power (ship-combat-power ship)))
    (let* ((credits-trend (- credits credits-prev))
           (power-trend (- fleet-power fleet-power-prev))
           (fleet-attrition (- fleet-power-prev fleet-power))
           (enemy-building-p (> enemy-count (floor enemy-count-prev)))
           (enemy-saving-p (>= enemy-stable-rounds 3.0))
           ;; Economic trajectory: 3-turn rolling average
           (credits-prev2 (slate-metric slate "credits-prev2" 0.0))
           (econ-delta-1 (- credits (floor credits-prev)))
           (econ-delta-2 (- (floor credits-prev) (floor credits-prev2)))
           (economic-trajectory (/ (+ econ-delta-1 econ-delta-2) 2.0))
           (economy-accelerating-p (> econ-delta-1 econ-delta-2))
           ;; Active theater states from persisted metrics
           (combats (slate-active-combats slate))
           (active-theater-states nil))
      ;; Classify each active engagement
      (dolist (combat combats)
        (let* ((hex (combat-hex combat))
               (our-p (slate-metric slate
                        (format nil "theater-~A-our-power" hex) 0.0))
               (their-p (slate-metric slate
                          (format nil "theater-~A-their-power" hex) 0.0))
               (rounds (floor (slate-metric slate
                                (format nil "theater-~A-rounds" hex) 0.0)))
               (state (cond
                        ((and (> our-p 0) (> their-p 0)
                              (> (/ our-p their-p) 1.3))
                         :winning)
                        ((and (> our-p 0) (> their-p 0)
                              (< (/ our-p their-p) 0.7))
                         :losing)
                        (t :stalled))))
          (push (list hex :state state :rounds rounds
                      :our-power our-p :their-power their-p)
                active-theater-states)))
      (list :turns-until-tech turns-until-tech
            :should-wait-for-tech-p should-wait
            :game-phase game-phase
            :credits-trend credits-trend
            :power-trend power-trend
            :fleet-attrition fleet-attrition
            :enemy-building-p enemy-building-p
            :enemy-saving-p enemy-saving-p
            :economic-trajectory economic-trajectory
            :economy-accelerating-p economy-accelerating-p
            :active-theater-states active-theater-states))))

;;; ============================================================================
;;; A7b: Theater Coordination (Section 8)
;;; ============================================================================

(defun compute-theater-priorities (slate strategy)
  "Rank theaters by strategic importance. Returns sorted (hex . score) alist.
   Scoring: hex-value + defense-bonus(50) + VP-scoring-bonus(40) +
   needs-reinforcement(30)."
  (let ((own-bases (slate-own-bases slate))
        (enemy-bases (slate-enemy-bases slate))
        (own-ships (slate-own-ships slate))
        (enemy-ships (slate-enemy-ships slate))
        (contested (slate-contested-hexes slate))
        (under-threat (getf strategy :bases-under-threat))
        (bases-held (getf strategy :bases-held))
        (hex-set nil)
        (results nil))
    ;; Collect all theater-relevant hexes
    (dolist (hex contested)
      (pushnew hex hex-set :test #'string-equal))
    (dolist (hex own-bases)
      (pushnew hex hex-set :test #'string-equal))
    (dolist (hex enemy-bases)
      (pushnew hex hex-set :test #'string-equal))
    ;; Score each theater
    (dolist (hex hex-set)
      (let ((score (compute-hex-value slate hex strategy)))
        ;; Defense bonus: own base under threat
        (when (and under-threat
                   (member hex under-threat :test #'string-equal))
          (incf score 50))
        ;; VP scoring bonus: enemy base we hold
        (when (and bases-held
                   (member hex bases-held :test #'string-equal))
          (incf score 40))
        ;; Needs reinforcement: our ships outnumbered here
        (let ((our-count (count-if (lambda (s) (string-equal (ship-hex s) hex))
                                   own-ships))
              (their-count (count-if (lambda (s) (string-equal (ship-hex s) hex))
                                     enemy-ships)))
          (when (and (> their-count 0) (< our-count their-count))
            (incf score 30)))
        (push (cons hex score) results)))
    ;; Sort descending by score
    (sort results (lambda (a b) (> (cdr a) (cdr b))))))

(defun compute-reinforcement-availability (slate strategy)
  "For each theater, which idle ships can reach it and in how many turns.
   Returns alist of (theater-hex . ((ship-code . turns-away) ...))."
  (let ((priorities (getf strategy :theater-priorities))
        (own-ships (slate-own-ships slate))
        (enemy-bases (slate-enemy-bases slate))
        (results nil))
    ;; Use theater priorities if available, otherwise contested hexes
    (let ((theater-hexes (if priorities
                             (mapcar #'car priorities)
                             (slate-contested-hexes slate))))
      (dolist (theater-hex theater-hexes)
        (let ((available nil))
          (dolist (ship own-ships)
            (when (and (ship-warpship-p ship)
                       (> (ship-pd ship) 0))
              (let ((shex (ship-hex ship)))
                (when (and shex
                           (not (string= shex ""))
                           (not (string-equal shex theater-hex))
                           ;; Ship is idle (not at enemy base, not in combat)
                           (not (member shex enemy-bases :test #'string-equal)))
                  (let* ((dist (slate-distance slate shex theater-hex))
                         (pd (ship-pd ship))
                         (turns-away (if (and (> pd 0) (< dist 999))
                                         (ceiling dist pd)
                                         999)))
                    (when (< turns-away 999)
                      (push (cons (ship-code ship) turns-away) available)))))))
          (when available
            (push (cons theater-hex
                        (sort available (lambda (a b) (< (cdr a) (cdr b)))))
                  results)))))
    results))

;;; ============================================================================
;;; A8: Build Directives
;;; ============================================================================

(defun compute-build-directives (slate strategy)
  "What to build and why. Derived from strategy state.
   No hardcoded cap. Build until fleet meets strategic need."
  (let* ((warp-count (getf strategy :warpship-count))
         (warp-needed (getf strategy :warpships-needed))
         (enemy-count (getf strategy :enemy-ship-count))
         (enemy-high-screen (getf strategy :enemy-high-screen-p))
         (unguarded (getf strategy :unguarded-own-bases))
         (ship-roles (getf strategy :ship-roles))
         (save-for-tech (getf strategy :should-wait-for-tech-p))
         ;; Fleet target: max of warpships-needed and enemy-count+1
         (target (max warp-needed (1+ enemy-count)))
         (ships-to-build (max 0 (- target warp-count)))
         ;; Count missile boats in fleet
         (missile-boat-count (count-if (lambda (r) (eq (cdr r) :missile-boat))
                                       ship-roles)))

    ;; Build type: warpships first until minimum fleet met, then defenders
    (let ((build-type (cond
                        ;; Warpships take priority until fleet requirement met
                        ((< warp-count warp-needed)
                         :warpship)
                        ;; Only build defenders after warpship needs are met
                        ((and unguarded (> (length unguarded) 0))
                         :systemship-defender)
                        (t :warpship))))
      ;; Design preference
      (let ((design-pref (cond
                           ((and enemy-high-screen (< missile-boat-count 1))
                            :missile-boat)
                           (t :brawler))))
        (list :ships-to-build ships-to-build
              :build-type build-type
              :design-preference design-pref
              :save-for-tech-p save-for-tech)))))

;;; ============================================================================
;;; A9: Movement Directives
;;; ============================================================================

(defun compute-movement-directives (slate strategy)
  "Where to send ships. Returns attack-assignments, defense-assignments,
   and idle-ships lists."
  (let* ((own-ships (slate-own-ships slate))
         (bases-needed (getf strategy :bases-needed))
         (unguarded (getf strategy :unguarded-own-bases))
         (under-threat (getf strategy :bases-under-threat))
         (posture (getf strategy :posture))
         (enemy-bases (slate-enemy-bases slate))
         ;; Collect idle warpships (not at enemy bases, has PD)
         (idle-warps nil)
         (attack-assignments nil)
         (defense-assignments nil)
         (idle-ships nil))

    ;; Gather idle warpships
    (dolist (ship own-ships)
      (when (and (ship-warpship-p ship)
                 (> (ship-pd ship) 0))
        (let ((hex (ship-hex ship))
              (at-enemy nil))
          (when (and hex (not (string= hex "")))
            (dolist (ehex enemy-bases)
              (when (string= hex ehex)
                (setf at-enemy t)))
            (unless at-enemy
              (push ship idle-warps))))))
    (setf idle-warps (nreverse idle-warps))

    ;; Defense assignments first if posture is defensive or bases threatened
    (when (or (eq posture :defensive) under-threat)
      (dolist (base-hex (intersection unguarded under-threat :test #'string=))
        (let ((best-ship nil)
              (best-dist 999))
          (dolist (ship idle-warps)
            (let ((dist (slate-distance slate (ship-hex ship) base-hex)))
              (when (< dist best-dist)
                (setf best-dist dist)
                (setf best-ship ship))))
          (when best-ship
            (push (cons (ship-code best-ship) base-hex) defense-assignments)
            (setf idle-warps (remove best-ship idle-warps))))))

    ;; Attack assignments: for each bases-needed, assign closest idle warpship
    (dolist (target-hex bases-needed)
      (let ((best-ship nil)
            (best-dist 999))
        (dolist (ship idle-warps)
          (let ((dist (slate-distance slate (ship-hex ship) target-hex)))
            (when (< dist best-dist)
              (setf best-dist dist)
              (setf best-ship ship))))
        (when best-ship
          (push (cons (ship-code best-ship) target-hex) attack-assignments)
          (setf idle-warps (remove best-ship idle-warps)))))

    ;; Redistribute remaining idle warps to highest-priority theaters
    (let ((theater-priorities (getf strategy :theater-priorities)))
      (when (and theater-priorities idle-warps)
        (dolist (tp theater-priorities)
          (when (null idle-warps)
            (return))
          (let* ((theater-hex (car tp))
                 (best-ship nil)
                 (best-dist 999))
            ;; Skip theaters that already have assignments
            (unless (or (find theater-hex attack-assignments
                              :test #'string-equal :key #'cdr)
                        (find theater-hex defense-assignments
                              :test #'string-equal :key #'cdr))
              (dolist (ship idle-warps)
                (let ((dist (slate-distance slate (ship-hex ship) theater-hex)))
                  (when (< dist best-dist)
                    (setf best-dist dist)
                    (setf best-ship ship))))
              (when (and best-ship (< best-dist 999))
                (push (cons (ship-code best-ship) theater-hex) attack-assignments)
                (setf idle-warps (remove best-ship idle-warps))))))))

    ;; Remaining idle warps get no assignment
    (dolist (ship idle-warps)
      (push (ship-code ship) idle-ships))

    (setf attack-assignments (nreverse attack-assignments))
    (setf defense-assignments (nreverse defense-assignments))
    (setf idle-ships (nreverse idle-ships))

    (list :attack-assignments attack-assignments
          :defense-assignments defense-assignments
          :idle-ships idle-ships)))

;;; ============================================================================
;;; A10: Posture
;;; ============================================================================

(defun compute-posture (slate)
  "Compute strategic posture by merging all ontology subsystems.
   Returns full strategy plist with :posture, :posture-reason, and all
   keys from victory, fleet, ship, enemy, theater, projection, temporal."
  (let* ((vstate (compute-victory-state slate))
         (fstate (compute-fleet-state slate))
         (sassess (compute-ship-assessments slate))
         (eprofile (compute-enemy-profile slate))
         (theater (compute-theater-map slate))
         (projection (compute-force-projection slate))
         (temporal (compute-temporal-state slate))
         (enemy-intent (compute-enemy-intent slate))
         (posture :balanced)
         (posture-reason "default balanced")
         ;; Extract keys for posture logic
         (fleet-power (getf fstate :fleet-combat-power))
         (enemy-power (getf fstate :enemy-combat-power))
         (score-state (getf vstate :score-state))
         (score-margin (getf vstate :score-margin))
         (tech-adv (getf fstate :tech-advantage))
         (vdist (getf vstate :victory-distance))
         (evdist (getf vstate :enemy-victory-distance)))

    ;; Rule 1: Base posture from combat power comparison
    (cond
      ((> fleet-power enemy-power)
       (setf posture :aggressive)
       (setf posture-reason "fleet combat power advantage"))
      ((< fleet-power enemy-power)
       (setf posture :defensive)
       (setf posture-reason "fleet combat power disadvantage"))
      (t
       (setf posture :balanced)
       (setf posture-reason "fleet combat power equal")))

    ;; Rule 2: Score state override — behind by 2+ VP forces aggressive
    (when (and (eq score-state :behind) (>= score-margin 2))
      (setf posture :aggressive)
      (setf posture-reason "behind by 2+ VP, must push"))

    ;; Rule 3: Tech advantage >= 1 leans aggressive
    (when (>= tech-adv 1.0)
      (when (not (eq posture :aggressive))
        (setf posture :aggressive)
        (setf posture-reason "tech advantage >= 1")))

    ;; Rule 4: Tech disadvantage >= 1 leans defensive
    (when (<= tech-adv -1.0)
      (when (not (eq posture :defensive))
        (setf posture :defensive)
        (setf posture-reason "tech disadvantage >= 1")))

    ;; Rule 5: K74 — victory-distance = 1 forces aggressive
    (when (= vdist 1)
      (setf posture :aggressive)
      (setf posture-reason "one base from victory"))

    ;; Rule 6: K75 — enemy-victory-distance <= 1 forces defensive (beats K74)
    (when (<= evdist 1)
      (setf posture :defensive)
      (setf posture-reason "enemy one base from winning"))

    ;; Rule 7: Counter-posture from opponent modeling (Section 9)
    ;; Only applies when we have actual intel (enemy-strategy != :unknown)
    (let ((enemy-strategy (getf enemy-intent :enemy-strategy)))
      (when (and enemy-strategy (not (eq enemy-strategy :unknown)))
        (cond
          ;; Enemy aggressive → consolidate defensively
          ((and (eq enemy-strategy :aggressive)
                (not (eq posture :defensive)))
           (setf posture :defensive)
           (setf posture-reason "counter: enemy aggressive, consolidating"))
          ;; Enemy tech-investing → push before they outtech us
          ((and (eq enemy-strategy :tech-investing)
                (not (eq posture :aggressive)))
           (setf posture :aggressive)
           (setf posture-reason "counter: enemy tech-investing, pushing")))))

    ;; Acceptable-loss threshold (Section 8: Theater Coordination)
    (let* ((warp-count (getf fstate :warpship-count))
           (warp-needed (getf fstate :warpships-needed))
           (surplus (- warp-count warp-needed))
           (acceptable-loss (cond ((<= surplus 0) 0)
                                  ((= surplus 1) 1)
                                  (t 2))))

    ;; Merge everything into one plist
    (let ((result (list :posture posture
                        :posture-reason posture-reason
                        :acceptable-loss-threshold acceptable-loss)))
      (setf result (append result vstate))
      (setf result (append result fstate))
      (setf result (append result sassess))
      (setf result (append result eprofile))
      (setf result (append result theater))
      (setf result (append result projection))
      (setf result (append result temporal))
      (setf result (append result enemy-intent))
      result))))

;;; ============================================================================
;;; A10b: Metrics Persistence
;;; ============================================================================

(defun compute-metrics-to-persist (slate strategy)
  "Emit all metric write-backs for cross-turn memory.
   Returns list of (make-metric name value) specs."
  (let ((metrics nil)
        (own-ships (slate-own-ships slate))
        (enemy-ships (slate-enemy-ships slate)))

    ;; Category 1: Trend snapshots
    (push (make-metric "credits-prev" (slate-credits slate)) metrics)
    (push (make-metric "fleet-count-prev" (length own-ships)) metrics)

    ;; Fleet power + attrition tracking
    (let ((fleet-power 0.0))
      (dolist (ship own-ships)
        (incf fleet-power (ship-combat-power ship)))
      (push (make-metric "fleet-power-prev" fleet-power) metrics)

      ;; Attrition: rolling 3-turn exponential average of fleet power delta
      (let* ((prev-power (slate-metric slate "fleet-power-prev" 0.0))
             (power-delta (- fleet-power prev-power))
             (prev-attrition (slate-metric slate "attrition-3turn-avg" 0.0))
             ;; Exponential moving average: alpha=0.4 (recent turns weighted more)
             (new-attrition (+ (* 0.4 power-delta) (* 0.6 prev-attrition))))
        (push (make-metric "attrition-3turn-avg" new-attrition) metrics)))

    ;; Enemy count + stable rounds counter
    (let* ((enemy-count (length enemy-ships))
           (prev-count (slate-metric slate "enemy-count-prev" 0.0))
           (prev-stable (slate-metric slate "enemy-count-stable-rounds" 0.0))
           (new-stable (if (= enemy-count (floor prev-count))
                           (1+ (floor prev-stable))
                           0)))
      (push (make-metric "enemy-count-prev" enemy-count) metrics)
      (push (make-metric "enemy-count-stable-rounds" new-stable) metrics))

    ;; Category 2: Enemy position snapshots
    (dolist (eship enemy-ships)
      (let* ((code (getf eship :code))
             (hex (ship-hex eship)))
        (when (and hex (not (string= hex "")))
          (let ((hex-num (parse-integer (strip-hex-prefix hex)
                                        :junk-allowed t)))
            (when hex-num
              (push (make-metric (format nil "enemy-~A-hex-prev" code)
                                 hex-num)
                    metrics))))))

    ;; Category 3: Enemy distance-to-base prev-snapshots
    (dolist (eship enemy-ships)
      (let ((code (getf eship :code)))
        (dolist (base (slate-own-bases slate))
          (let ((dist (slate-enemy-distance slate code base)))
            (when (< dist 999)
              (push (make-metric (format nil "enemy-~A-dist-~A" code base)
                                 dist)
                    metrics))))))

    ;; Category 4: Combat order watermarks
    (let ((intent (getf strategy :enemy-ship-profiles)))
      (dolist (profile intent)
        (when (listp profile)
          (let ((code (first profile))
                (plist (rest profile)))
            (when (getf plist :max-pd)
              (push (make-metric (format nil "enemy-~A-max-pd-seen" code)
                                 (getf plist :max-pd))
                    metrics))
            (when (getf plist :max-beam)
              (push (make-metric (format nil "enemy-~A-max-beam-seen" code)
                                 (getf plist :max-beam))
                    metrics))
            (when (getf plist :max-screen)
              (push (make-metric
                     (format nil "enemy-~A-max-screen-seen" code)
                     (getf plist :max-screen))
                    metrics))
            (when (getf plist :max-tube)
              (push (make-metric (format nil "enemy-~A-max-tube-seen" code)
                                 (getf plist :max-tube))
                    metrics))
            ;; Category 5: Rack inference
            (when (getf plist :has-racks)
              (push (make-metric
                     (format nil "enemy-~A-has-racks" code) 1)
                    metrics))))))

    ;; Category 6: Per-theater engagement history
    (let ((combats (slate-active-combats slate)))
      (dolist (combat combats)
        (let* ((hex (combat-hex combat))
               (own-here (ships-at-hex (slate-own-ships slate) hex))
               (enemy-here (ships-at-hex (slate-enemy-ships slate) hex))
               (our-power 0.0)
               (their-power 0.0)
               (prev-rounds (slate-metric slate
                              (format nil "theater-~A-rounds" hex) 0.0)))
          (dolist (s own-here)
            (incf our-power (ship-combat-power s)))
          (dolist (s enemy-here)
            (incf their-power (ship-combat-power s)))
          (push (make-metric (format nil "theater-~A-rounds" hex)
                             (1+ (floor prev-rounds)))
                metrics)
          (push (make-metric (format nil "theater-~A-our-power" hex)
                             our-power)
                metrics)
          (push (make-metric (format nil "theater-~A-their-power" hex)
                             their-power)
                metrics))))

    ;; Category 7: Economic trajectory (chain credits-prev2)
    (let ((credits-prev (slate-metric slate "credits-prev" 0.0)))
      (push (make-metric "credits-prev2" credits-prev) metrics))

    ;; Category 8: Enemy economic estimate
    (let ((enemy-bp-est (getf strategy :enemy-total-bp-est)))
      (when enemy-bp-est
        (push (make-metric "enemy-total-bp-est" enemy-bp-est) metrics)))

    (nreverse metrics)))

;;; ============================================================================
;;; A11: Top-Level Entry Point
;;; ============================================================================

(defun compute-strategic-state (slate)
  "Single call: computes the complete strategy plist.
   Calls all subsystems: posture → theater coordination → build → movement."
  (let* ((strategy (compute-posture slate))
         ;; Theater coordination (Section 8)
         (force-ratios (compute-theater-force-ratios slate))
         (theater-priorities (compute-theater-priorities slate strategy))
         (strategy (append strategy
                          (list :theater-force-ratios force-ratios
                                :theater-priorities theater-priorities)))
         (reinforcements (compute-reinforcement-availability slate strategy))
         (strategy (append strategy
                          (list :reinforcement-availability reinforcements)))
         ;; Build and movement directives
         (build-dirs (compute-build-directives slate strategy))
         (move-dirs (compute-movement-directives slate strategy)))
    (append strategy build-dirs move-dirs)))
