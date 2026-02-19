;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-rules.lisp - Gen4 Rule Engine and Strategy Rule Declarations
;;;;
;;;; The rule engine dispatches named rules by phase.
;;;; Three engine variants:
;;;;   fire-first-matching-rule — first :when match wins
;;;;   fire-first-producing-rule — first :when match with non-nil :action wins
;;;;   fire-posture-rules — last match wins (sequential override semantics)
;;;;
;;;; Rule declarations are organized by phase:
;;;;   :posture, :build, :design, :move, :combat, :triage, :tactics, :pickdrop
;;;;
;;;; Depends on: aa-macros.lisp (define-strategy-rule macro, *strategy-rules*)
;;;;             aa-theta.lisp (theta parameters)
;;;;             aa-entities.lisp (entity accessors)
;;;;             aa-util.lisp (slate accessors, helpers)
;;;;
;;;; Functions from later files (aa-build, aa-movement, aa-combat, aa-core)
;;;; are referenced by name in rule lambdas; resolved at call time.

;;; ============================================================================
;;; Clear Registry (safe for reload)
;;; ============================================================================

(setf *strategy-rules* nil)

;;; ============================================================================
;;; Rule Engine Infrastructure
;;; ============================================================================

(defun fire-first-matching-rule (phase slate strategy)
  "Collect rules for PHASE, sort by priority ascending, fire first match.
   Returns result of first matching rule's action.
   Falls back to (cmd-next) if no rule matches."
  (let* ((phase-rules (remove-if-not
                        (lambda (r) (eq (getf r :phase) phase))
                        *strategy-rules*))
         (sorted (sort (copy-list phase-rules)
                       (lambda (a b) (< (getf a :priority)
                                        (getf b :priority))))))
    (dolist (rule sorted)
      (when (funcall (getf rule :when) slate strategy)
        (push (getf rule :name) *fired-rules-this-cycle*)
        (return-from fire-first-matching-rule
          (funcall (getf rule :action) slate strategy))))
    (list (cmd-next))))

(defun fire-first-producing-rule (phase slate strategy)
  "Variant: first matching rule whose action returns non-NIL wins.
   If a matched rule's action returns NIL, continue to next rule.
   Used for phases where :when is broad and :action may decline."
  (let* ((phase-rules (remove-if-not
                        (lambda (r) (eq (getf r :phase) phase))
                        *strategy-rules*))
         (sorted (sort (copy-list phase-rules)
                       (lambda (a b) (< (getf a :priority)
                                        (getf b :priority))))))
    (dolist (rule sorted)
      (when (funcall (getf rule :when) slate strategy)
        (let ((result (funcall (getf rule :action) slate strategy)))
          (when result
            (push (getf rule :name) *fired-rules-this-cycle*)
            (return-from fire-first-producing-rule result)))))
    (list (cmd-next))))

(defun fire-posture-rules (slate strategy)
  "Last-match-wins: evaluate all posture rules in priority order.
   Passes :current-posture through strategy so later rules can check
   earlier results (replicating sequential override semantics).
   Returns (values posture reason)."
  (let* ((posture :balanced)
         (reason "default balanced")
         (phase-rules (remove-if-not
                        (lambda (r) (eq (getf r :phase) :posture))
                        *strategy-rules*))
         (sorted (sort (copy-list phase-rules)
                       (lambda (a b) (< (getf a :priority)
                                        (getf b :priority))))))
    (dolist (rule sorted)
      (let ((augmented (append strategy (list :current-posture posture))))
        (when (funcall (getf rule :when) slate augmented)
          (let ((result (funcall (getf rule :action) slate augmented)))
            (setf posture (first result))
            (setf reason (second result))
            (push (getf rule :name) *fired-rules-this-cycle*)))))
    (values posture reason)))

(defun reset-cycle-trace ()
  "Clear rule trace for new CALCULATE cycle."
  (setf *fired-rules-this-cycle* nil))

(defun fired-rules-this-cycle ()
  "Return list of rule names fired this cycle."
  *fired-rules-this-cycle*)

;;; ============================================================================
;;; Posture Rules (from compute-posture cascade, last-match-wins)
;;; ============================================================================
;;; Strategy plist keys: :fleet-power :enemy-power :score-state :score-margin
;;;   :tech-adv :vdist :evdist :enemy-strategy :current-posture

(define-strategy-rule posture-fleet-advantage
  :phase :posture :priority 100
  :when (> (getf strategy :fleet-power) (getf strategy :enemy-power))
  :action (list :aggressive "fleet combat power advantage")
  :doc "Fleet power > enemy -> aggressive.")

(define-strategy-rule posture-fleet-disadvantage
  :phase :posture :priority 200
  :when (< (getf strategy :fleet-power) (getf strategy :enemy-power))
  :action (list :defensive "fleet combat power disadvantage")
  :doc "Fleet power < enemy -> defensive.")

(define-strategy-rule posture-fleet-equal
  :phase :posture :priority 300
  :when (= (getf strategy :fleet-power) (getf strategy :enemy-power))
  :action (list :balanced "fleet combat power equal")
  :doc "Fleet power = enemy -> balanced.")

(define-strategy-rule posture-vp-behind
  :phase :posture :priority 400
  :when (and (eq (getf strategy :score-state) :behind)
             (>= (getf strategy :score-margin)
                  (theta 'theta-posture-vp-behind)))
  :action (list :aggressive "behind by 2+ VP, must push")
  :doc "Behind in VP -> aggressive.")

(define-strategy-rule posture-tech-advantage
  :phase :posture :priority 500
  :when (and (>= (getf strategy :tech-adv)
                  (theta 'theta-posture-tech-aggressive))
             (not (eq (getf strategy :current-posture) :aggressive)))
  :action (list :aggressive "tech advantage >= 1")
  :doc "Tech advantage -> aggressive (unless already).")

(define-strategy-rule posture-tech-disadvantage
  :phase :posture :priority 600
  :when (and (<= (getf strategy :tech-adv)
                  (theta 'theta-posture-tech-defensive))
             (not (eq (getf strategy :current-posture) :defensive)))
  :action (list :defensive "tech disadvantage >= 1")
  :doc "Tech disadvantage -> defensive (unless already).")

(define-strategy-rule posture-near-victory
  :phase :posture :priority 700
  :when (= (getf strategy :vdist) 1)
  :action (list :aggressive "one base from victory")
  :doc "One base from victory -> aggressive.")

(define-strategy-rule posture-enemy-near-victory
  :phase :posture :priority 800
  :when (<= (getf strategy :evdist) 1)
  :action (list :defensive "enemy one base from winning")
  :doc "Enemy near victory -> defensive.")

(define-strategy-rule posture-counter-aggressive
  :phase :posture :priority 900
  :when (let ((es (getf strategy :enemy-strategy)))
          (and es (not (eq es :unknown))
               (eq es :aggressive)
               (not (eq (getf strategy :current-posture) :defensive))))
  :action (list :defensive "counter: enemy aggressive, consolidating")
  :doc "Counter enemy aggression -> defensive.")

(define-strategy-rule posture-counter-tech-invest
  :phase :posture :priority 1000
  :when (let ((es (getf strategy :enemy-strategy)))
          (and es (not (eq es :unknown))
               (eq es :tech-investing)
               (not (eq (getf strategy :current-posture) :aggressive))))
  :action (list :aggressive "counter: enemy tech-investing, pushing")
  :doc "Counter enemy tech-investing -> aggressive.")

;;; ============================================================================
;;; Build Phase Rules (from decide-build-phase cond, first-producing)
;;; ============================================================================

(define-strategy-rule build-commit-draft
  :phase :build :priority 100
  :when (draft-ready-p (slate-drafts slate))
  :action (let ((name (ship-name (first (slate-drafts slate)))))
            (format t "[LISP] -> bc ~A~%" name)
            (list (make-cmd "bc" name)))
  :doc "Commit draft with specs.")

(define-strategy-rule build-set-specs
  :phase :build :priority 200
  :when (draft-needs-specs-p (slate-drafts slate))
  :action (let* ((drafts (slate-drafts slate))
                 (name (ship-name (first drafts)))
                 (spec (choose-ship-design slate strategy)))
            (format t "[LISP] -> bs ~A ~A~%" name spec)
            (list (make-cmd "bs" (format nil "~A ~A" name spec))))
  :doc "Set specs on draft.")

(define-strategy-rule build-deploy-ship
  :phase :build :priority 300
  :when (ship-needs-deploy-p (slate-own-ships slate))
  :action (let* ((ship (find-undeployed-ship (slate-own-ships slate)))
                 (base (ensure-hex-prefix
                        (choose-deploy-base slate (slate-own-ships slate) strategy))))
            (format t "[LISP] -> ds ~A to ~A~%" (ship-name ship) base)
            (list (make-cmd "ds" (format nil "~A ~A" (ship-code ship) base))))
  :doc "Deploy undeployed ship.")

(define-strategy-rule build-repair
  :phase :build :priority 400
  :when (should-repair-p slate)
  :action (issue-repair-command slate)
  :doc "Repair damaged ship at base.")

(define-strategy-rule build-economic
  :phase :build :priority 500
  :when t
  :action (let ((cmds (decide-economic-actions slate)))
            (when cmds
              (format t "[LISP] -> economic action~%")
              cmds))
  :doc "Economic action if available (nil falls through).")

(define-strategy-rule build-defender
  :phase :build :priority 600
  :when (should-build-defender-p slate strategy)
  :action (let ((name (next-ship-name)))
            (format t "[LISP] -> bn s ~A (base defender)~%" name)
            (list (make-cmd "bn" (format nil "s ~A" name))))
  :doc "Build systemship defender.")

(define-strategy-rule build-warpship
  :phase :build :priority 700
  :when (should-build-p slate strategy)
  :action (let ((name (next-ship-name)))
            (format t "[LISP] -> bn w ~A~%" name)
            (list (make-cmd "bn" (format nil "w ~A" name))))
  :doc "Build new warpship.")

(define-strategy-rule build-done
  :phase :build :priority 800
  :when t
  :action (progn
            (format t "[LISP] -> NEXT~%")
            (list (cmd-next)))
  :doc "Build phase complete.")

;;; ============================================================================
;;; Ship Design Rules (from choose-ship-design cond, first-match)
;;; ============================================================================
;;; Strategy augmented with :design-drafts for systemship check.

(define-strategy-rule design-systemship-defender
  :phase :design :priority 100
  :when (let ((drafts (slate-drafts slate)))
          (and drafts (is-draft-systemship-p (first drafts))))
  :action (progn
            (format t "[LISP] Designing systemship defender~%")
            (get-ship-template :defender))
  :doc "Systemship draft -> defender template.")

(define-strategy-rule design-first-ship-brawler
  :phase :design :priority 200
  :when (null (slate-own-ships slate))
  :action (progn
            (format t "[LISP] First ship - building brawler~%")
            (get-ship-template :brawler))
  :doc "First ship -> brawler.")

(define-strategy-rule design-strategy-torpedo-boat
  :phase :design :priority 300
  :when (and strategy
             (eq (getf strategy :design-preference) :torpedo-boat)
             (< (count-ship-type (slate-own-ships slate) :torpedo-boat)
                (theta 'theta-torpedo-boats-cap)))
  :action (progn
            (format t "[LISP] Strategy: building torpedo boat (enemy high shield)~%")
            (get-ship-template :torpedo-boat))
  :doc "Strategy requests torpedo-boat.")

(define-strategy-rule design-enemy-high-shield
  :phase :design :priority 400
  :when (and (if strategy
                 (getf strategy :enemy-high-shield-p)
                 (enemy-heavy-shields-p (slate-enemy-ships slate)))
             (< (count-ship-type (slate-own-ships slate) :torpedo-boat)
                (theta 'theta-torpedo-boats-cap)))
  :action (progn
            (format t "[LISP] Enemy has heavy shields - building torpedo boat~%")
            (get-ship-template :torpedo-boat))
  :doc "Enemy heavy shields -> torpedo-boat.")

(define-strategy-rule design-high-tech-brawler
  :phase :design :priority 500
  :when (and (>= (slate-tech-level slate) (theta 'theta-design-tech-threshold))
             (< (count-ship-type (slate-own-ships slate) :brawler)
                (theta 'theta-brawler-tech-cap)))
  :action (progn
            (format t "[LISP] Tech ~A - building brawler~%" (slate-tech-level slate))
            (get-ship-template :brawler))
  :doc "High tech -> brawler.")

(define-strategy-rule design-need-interceptors
  :phase :design :priority 600
  :when (< (count-ship-type (slate-own-ships slate) :interceptor)
           (theta 'theta-interceptor-cap))
  :action (progn
            (format t "[LISP] Fleet needs interceptors~%")
            (get-ship-template :interceptor))
  :doc "Fleet needs interceptors.")

(define-strategy-rule design-fleet-variety
  :phase :design :priority 700
  :when (and (>= (length (slate-own-ships slate)) (theta 'theta-fleet-variety-size))
             (< (count-ship-type (slate-own-ships slate) :torpedo-boat)
                (theta 'theta-torpedo-boats-cap)))
  :action (progn
            (format t "[LISP] Adding torpedo boat to fleet mix~%")
            (get-ship-template :torpedo-boat))
  :doc "Fleet variety -> torpedo-boat.")

(define-strategy-rule design-default-brawler
  :phase :design :priority 800
  :when t
  :action (get-ship-template :brawler)
  :doc "Default design: brawler.")

;;; ============================================================================
;;; Movement Rules (from decide-movement-phase, first-producing)
;;; ============================================================================

(define-strategy-rule move-defense
  :phase :move :priority 100
  :when (and strategy (getf strategy :defense-assignments))
  :action (let ((round (slate-round slate)))
            (block found
              (dolist (assignment (getf strategy :defense-assignments))
                (let* ((scode (car assignment))
                       (target-hex (cdr assignment))
                       (ship (find-ship-by-code (slate-own-ships slate) scode)))
                  (when (and ship
                             (ship-warpship-p ship)
                             (> (ship-pd ship) 0)
                             (not (string= (ship-hex ship) target-hex)))
                    (format t "[LISP] -> defense move ~A to ~A~%"
                            (ship-name ship) target-hex)
                    (return-from found
                      (list (make-cmd "m" (format nil "~A ~A"
                                                   (ship-code ship)
                                                   (ensure-hex-prefix target-hex))))))))
              nil))
  :doc "Move ship per defense assignment.")

(define-strategy-rule move-attack
  :phase :move :priority 200
  :when (and strategy (getf strategy :attack-assignments))
  :action (let ((round (slate-round slate)))
            (block found
              (dolist (assignment (getf strategy :attack-assignments))
                (let* ((scode (car assignment))
                       (target-hex (cdr assignment))
                       (ship (find-ship-by-code (slate-own-ships slate) scode)))
                  (when (and ship
                             (ship-warpship-p ship)
                             (> (ship-pd ship) 0)
                             (not (string= (ship-hex ship) target-hex)))
                    ;; Turn 1 restriction
                    (when (and (= round 1) (is-enemy-base-p target-hex slate))
                      (format t "[LISP] Turn 1: can't move to enemy base~%")
                      (return-from found (list (cmd-next))))
                    ;; Use suggested-dest if available
                    (let ((dest (ship-suggested-dest ship)))
                      (if (and dest (not (string= dest "")))
                          (progn
                            (format t "[LISP] -> attack move ~A to ~A (via ~A)~%"
                                    (ship-name ship) target-hex dest)
                            (return-from found
                              (list (make-cmd "m" (format nil "~A ~A"
                                                           (ship-code ship)
                                                           (ensure-hex-prefix dest))))))
                          (progn
                            (format t "[LISP] -> attack move ~A to ~A (direct)~%"
                                    (ship-name ship) target-hex)
                            (return-from found
                              (list (make-cmd "m" (format nil "~A ~A"
                                                           (ship-code ship)
                                                           (ensure-hex-prefix target-hex)))))))))))
              nil))
  :doc "Move ship per attack assignment.")

(define-strategy-rule move-fallback
  :phase :move :priority 300
  :when t
  :action (let ((ship (find-ship-to-move slate)))
            (when ship
              (let ((dest (ensure-hex-prefix (ship-suggested-dest ship)))
                    (round (slate-round slate)))
                (if (and (= round 1) (is-enemy-base-p dest slate))
                    (progn
                      (format t "[LISP] Turn 1: can't move to enemy base, skip~%")
                      (list (cmd-next)))
                    (progn
                      (format t "[LISP] -> move ~A to ~A~%" (ship-name ship) dest)
                      (list (make-cmd "m" (format nil "~A ~A"
                                                   (ship-code ship) dest))))))))
  :doc "Fallback: move ship with suggested destination.")

(define-strategy-rule move-done
  :phase :move :priority 400
  :when t
  :action (progn
            (format t "[LISP] -> NEXT (no ships can move)~%")
            (list (cmd-next)))
  :doc "No movement actions.")

;;; ============================================================================
;;; Combat Phase Rules (from decide-combat-phase cond, first-match)
;;; ============================================================================
;;; Strategy augmented with :combat-focus, :combat-focus-hex,
;;;   :combat-ships-in-focus, :combat-theater-decision, :combat-triage

(define-strategy-rule combat-escape
  :phase :combat :priority 100
  :when (find-escaping-ship (slate-own-ships slate))
  :action (let ((ship (find-escaping-ship (slate-own-ships slate))))
            (format t "[LISP] -> retreat ~A~%" (ship-name ship))
            (issue-retreat ship slate strategy))
  :doc "Handle escaping ship.")

(define-strategy-rule combat-damage
  :phase :combat :priority 200
  :when (find-ship-with-damage (slate-own-ships slate))
  :action (let ((ship (find-ship-with-damage (slate-own-ships slate))))
            (format t "[LISP] -> apply damage to ~A~%" (ship-name ship))
            (issue-damage-assignment ship))
  :doc "Assign pending damage.")

(define-strategy-rule combat-issue-order
  :phase :combat :priority 300
  :when (and (getf strategy :combat-focus-hex)
             (find-ship-needing-order (getf strategy :combat-ships-in-focus)))
  :action (let* ((ships-in-focus (getf strategy :combat-ships-in-focus))
                 (ship (find-ship-needing-order ships-in-focus))
                 (focus-hex (getf strategy :combat-focus-hex))
                 (focus-combat (getf strategy :combat-focus))
                 (enemies-here (ships-at-hex (slate-enemy-ships slate) focus-hex))
                 (focus-target (pick-focus-target enemies-here))
                 (stalemate (combat-stalemate-count focus-combat))
                 (ai-attacker (combat-ai-attacker-p focus-combat))
                 (theater-decision (getf strategy :combat-theater-decision)))
            (when (and ai-attacker (>= stalemate 2))
              (format t "[LISP] WARNING: Stalemate=~A, must deal damage or retreat!~%"
                      stalemate))
            (format t "[LISP] -> combat order for ~A in ~A (focus: ~A stalemate: ~A triage: ~A)~%"
                    (ship-name ship) focus-hex
                    (when focus-target (ship-code focus-target))
                    stalemate theater-decision)
            (issue-combat-order-with-triage ship focus-target enemies-here
                                            stalemate ai-attacker theater-decision
                                            strategy))
  :doc "Issue combat order for ship in focus hex.")

(define-strategy-rule combat-commit
  :phase :combat :priority 400
  :when (let ((combats (slate-active-combats slate))
              (focus-hex (getf strategy :combat-focus-hex))
              (ships-in-focus (getf strategy :combat-ships-in-focus)))
          (needs-combat-commit-p combats ships-in-focus focus-hex))
  :action (progn
            (format t "[LISP] -> cc~%")
            (list (make-cmd "cc")))
  :doc "Commit combat orders.")

(define-strategy-rule combat-waiting
  :phase :combat :priority 500
  :when (and (slate-active-combats slate)
             (not (getf strategy :combat-focus-hex)))
  :action (progn
            (format t "[LISP] -> WAIT (combat active, awaiting enemy/user)~%")
            nil)
  :doc "Waiting for enemy/user action.")

(define-strategy-rule combat-voluntary-evaluate
  :phase :combat :priority 550
  :when (and (not (slate-active-combats slate))
             (getf strategy :voluntary-combat-hexes))
  :action (or (evaluate-and-initiate-voluntary-combat slate strategy)
              (progn
                (format t "[LISP] -> NEXT (voluntary: no engagement)~%")
                (list (cmd-next))))
  :doc "No mandatory combats: evaluate contested hexes for voluntary engagement.")

(define-strategy-rule combat-done
  :phase :combat :priority 600
  :when (not (slate-active-combats slate))
  :action (progn
            (format t "[LISP] -> NEXT (no combat)~%")
            (list (cmd-next)))
  :doc "No active combats.")

;;; ============================================================================
;;; Theater Triage Rules (from assess-theater cond, first-match)
;;; ============================================================================
;;; Strategy keys: :triage-force-ratio :triage-is-vp-hex :triage-can-penetrate
;;;   :triage-have-warpship :triage-acceptable-loss

(define-strategy-rule triage-vp-fight
  :phase :triage :priority 100
  :when (and (getf strategy :triage-is-vp-hex)
             (>= (getf strategy :triage-force-ratio)
                  (theta 'theta-triage-vp-fight-ratio)))
  :action :fight
  :doc "VP hex with OK force ratio -> fight.")

(define-strategy-rule triage-vp-hold
  :phase :triage :priority 200
  :when (and (getf strategy :triage-is-vp-hex)
             (< (getf strategy :triage-force-ratio)
                (theta 'theta-triage-vp-fight-ratio)))
  :action :hold
  :doc "VP hex badly outmatched -> hold.")

(define-strategy-rule triage-advantage-penetrate
  :phase :triage :priority 300
  :when (and (>= (getf strategy :triage-force-ratio)
                  (theta 'theta-triage-advantage-ratio))
             (getf strategy :triage-can-penetrate))
  :action :fight
  :doc "Advantage + can penetrate -> fight.")

(define-strategy-rule triage-acceptable-loss
  :phase :triage :priority 400
  :when (and (> (getf strategy :triage-acceptable-loss) 0)
             (not (getf strategy :triage-is-vp-hex))
             (>= (getf strategy :triage-force-ratio)
                  (theta 'theta-triage-acceptable-fight-ratio)))
  :action (progn
            (format t "[LISP] Acceptable loss threshold allows fighting~%")
            :fight)
  :doc "Acceptable loss at low-value hex -> fight.")

(define-strategy-rule triage-advantage-no-penetrate
  :phase :triage :priority 500
  :when (and (>= (getf strategy :triage-force-ratio)
                  (theta 'theta-triage-advantage-ratio))
             (not (getf strategy :triage-can-penetrate)))
  :action (progn
            (format t "[LISP] Can't penetrate shields despite advantage - holding~%")
            :hold)
  :doc "Advantage but can't penetrate -> hold.")

(define-strategy-rule triage-matched
  :phase :triage :priority 600
  :when (>= (getf strategy :triage-force-ratio)
             (theta 'theta-triage-matched-ratio))
  :action :hold
  :doc "Matched forces -> hold.")

(define-strategy-rule triage-outmatched-retreat
  :phase :triage :priority 700
  :when (and (getf strategy :triage-have-warpship)
             (< (getf strategy :triage-force-ratio)
                (theta 'theta-triage-retreat-ratio)))
  :action :retreat
  :doc "Outmatched with warpship -> retreat.")

(define-strategy-rule triage-default-hold
  :phase :triage :priority 800
  :when t
  :action :hold
  :doc "Default: hold.")

;;; ============================================================================
;;; Combat Tactics Rules (from issue-combat-order-with-triage +
;;;   analyze-combat-situation, first-match)
;;; ============================================================================
;;; Strategy keys: :tactics-ship :tactics-target :tactics-enemies
;;;   :tactics-stalemate :tactics-ai-attacker :tactics-triage-decision
;;;   Plus main strategy keys: :endgame-p :vp-race-winner :bases-held
;;;   :risk-tolerance

(define-strategy-rule tactics-endgame-defend
  :phase :tactics :priority 100
  :when (let* ((ship (getf strategy :tactics-ship))
               (hex (ship-hex ship))
               (bases-held (getf strategy :bases-held)))
          (and (getf strategy :endgame-p)
               (eq (getf strategy :vp-race-winner) :us)
               bases-held
               (member hex bases-held :test #'string-equal)))
  :action (let ((ship (getf strategy :tactics-ship)))
            (format t "[LISP] Endgame: winning VP race, defending held base~%")
            (crt-dodge-alloc (ship-pd ship) (ship-phasic ship) (ship-shield ship)))
  :doc "Endgame: winning VP, dodge-defend held base.")

(define-strategy-rule tactics-endgame-attack
  :phase :tactics :priority 200
  :when (and (getf strategy :endgame-p)
             (eq (getf strategy :vp-race-winner) :them))
  :action (let* ((ship (getf strategy :tactics-ship))
                 (target (getf strategy :tactics-target)))
            (format t "[LISP] Endgame: enemy winning VP race, maximum aggression~%")
            (crt-attack-alloc (ship-pd ship) (ship-phasic ship) (ship-shield ship)
                              (if target (or (ship-last-drive target) 0) 0)
                              :standard))
  :doc "Endgame: enemy winning VP, max aggression.")

(define-strategy-rule tactics-auto-retreat
  :phase :tactics :priority 300
  :when (let ((ship (getf strategy :tactics-ship))
              (risk (or (getf strategy :risk-tolerance) :normal)))
          (should-auto-retreat-p ship risk))
  :action (let ((ship (getf strategy :tactics-ship)))
            (format t "[LISP] Auto-retreat: ship ~A below HP threshold~%"
                    (ship-name ship))
            (list :tactic "e"
                  :alloc (list :d (ship-pd ship) :b 0 :s 0)
                  :torpedoes nil))
  :doc "Damaged ship below retreat threshold -> escape.")

(define-strategy-rule tactics-stalemate-must-damage
  :phase :tactics :priority 400
  :when (and (getf strategy :tactics-ai-attacker)
             (>= (getf strategy :tactics-stalemate) 1))
  :action (let* ((ship (getf strategy :tactics-ship))
                 (target (getf strategy :tactics-target))
                 (enemies (getf strategy :tactics-enemies)))
            (format t "[LISP] Stalemate override: must damage or retreat~%")
            (analyze-must-damage-situation ship target enemies))
  :doc "Stalemate danger: must deal damage or retreat.")

(define-strategy-rule tactics-triage-retreat
  :phase :tactics :priority 500
  :when (and (eq (getf strategy :tactics-triage-decision) :retreat)
             (ship-warpship-p (getf strategy :tactics-ship)))
  :action (let ((ship (getf strategy :tactics-ship)))
            (format t "[LISP] Triage: RETREAT - preserving ship for better fights~%")
            (list :tactic "e"
                  :alloc (list :d (ship-pd ship) :b 0 :s 0)
                  :torpedoes nil))
  :doc "Triage says retreat -> escape.")

(define-strategy-rule tactics-triage-hold
  :phase :tactics :priority 600
  :when (eq (getf strategy :tactics-triage-decision) :hold)
  :action (let ((ship (getf strategy :tactics-ship)))
            (format t "[LISP] Triage: HOLD - dodging defensively~%")
            (crt-dodge-alloc (ship-pd ship) (ship-phasic ship) (ship-shield ship)))
  :doc "Triage says hold -> dodge.")

(define-strategy-rule tactics-torpedo-boat-fire
  :phase :tactics :priority 700
  :when (let ((ship (getf strategy :tactics-ship)))
          (and (eq (classify-ship-role ship) :torpedo-boat)
               (> (ship-launcher ship) 0)
               (> (ship-torpedo ship) 0)))
  :action (let* ((ship (getf strategy :tactics-ship))
                 (target (getf strategy :tactics-target)))
            (format t "[LISP] Torpedo-boat ~A: firing torpedoes~%" (ship-name ship))
            (crt-torpedo-alloc (ship-pd ship) (ship-launcher ship) (ship-torpedo ship)
                               (ship-tech ship)
                               (if target (or (ship-last-drive target) 0) 0)))
  :doc "Torpedo-boat with ammo -> fire torpedoes.")

(define-strategy-rule tactics-torpedo-boat-retreat
  :phase :tactics :priority 800
  :when (let ((ship (getf strategy :tactics-ship)))
          (and (eq (classify-ship-role ship) :torpedo-boat)
               (or (= (ship-launcher ship) 0) (= (ship-torpedo ship) 0))
               (ship-warpship-p ship)))
  :action (let ((ship (getf strategy :tactics-ship)))
            (format t "[LISP] Torpedo-boat ~A: out of torpedoes, retreating~%"
                    (ship-name ship))
            (list :tactic "e"
                  :alloc (list :d (ship-pd ship) :b 0 :s 0)
                  :torpedoes nil))
  :doc "Torpedo-boat empty -> retreat.")

(define-strategy-rule tactics-fortress-attack
  :phase :tactics :priority 900
  :when (eq (classify-ship-role (getf strategy :tactics-ship)) :fortress)
  :action (let* ((ship (getf strategy :tactics-ship))
                 (target (getf strategy :tactics-target)))
            (format t "[LISP] Fortress ~A: all-in attack~%" (ship-name ship))
            (crt-attack-alloc (ship-pd ship) (ship-phasic ship) (ship-shield ship)
                              (if target (or (ship-last-drive target) 0) 0)
                              :standard))
  :doc "Fortress -> all-in attack.")

(define-strategy-rule tactics-alpha-strike
  :phase :tactics :priority 1000
  :when (let* ((ship (getf strategy :tactics-ship))
               (target (getf strategy :tactics-target)))
          (and target
               (> (ship-launcher ship) 0)
               (> (ship-torpedo ship) 0)
               (null (ship-last-tactic target))
               (> (+ (or (ship-shield target) 0) (or (ship-tech target) 0))
                  (theta 'theta-alpha-strike-shield-threshold))))
  :action (let* ((ship (getf strategy :tactics-ship))
                 (target (getf strategy :tactics-target)))
            (format t "[LISP] Torpedo alpha strike (enemy shields ~A+~A)~%"
                    (or (ship-shield target) 0) (or (ship-tech target) 0))
            (crt-torpedo-alloc (ship-pd ship) (ship-launcher ship) (ship-torpedo ship)
                               (ship-tech ship)
                               (or (ship-last-drive target) 0)))
  :doc "Alpha strike against heavy shields.")

(define-strategy-rule tactics-outmatched-retreat
  :phase :tactics :priority 1100
  :when (let* ((ship (getf strategy :tactics-ship))
               (enemies (getf strategy :tactics-enemies))
               (total-enemy-pd (reduce #'+ (mapcar #'ship-pd enemies)
                                       :initial-value 0)))
          (and (< (ship-pd ship)
                  (* total-enemy-pd (theta 'theta-combat-outmatched-ratio)))
               (ship-warpship-p ship)))
  :action (let* ((ship (getf strategy :tactics-ship))
                 (enemies (getf strategy :tactics-enemies))
                 (total-enemy-pd (reduce #'+ (mapcar #'ship-pd enemies)
                                         :initial-value 0)))
            (format t "[LISP] Outmatched (~A vs ~A total) - retreating~%"
                    (ship-pd ship) total-enemy-pd)
            (list :tactic "e"
                  :alloc (list :d (ship-pd ship) :b 0 :s 0)
                  :torpedoes nil))
  :doc "Badly outmatched -> retreat.")

(define-strategy-rule tactics-enemy-retreating
  :phase :tactics :priority 1200
  :when (let ((target (getf strategy :tactics-target)))
          (and target (eql (ship-last-tactic target) #\E)))
  :action (let* ((ship (getf strategy :tactics-ship))
                 (target (getf strategy :tactics-target)))
            (format t "[LISP] Enemy retreating - attacking to pursue~%")
            (crt-attack-alloc (ship-pd ship) (ship-phasic ship) (ship-shield ship)
                              (or (ship-last-drive target) 0) :pursue))
  :doc "Enemy retreating -> pursue attack.")

(define-strategy-rule tactics-enemy-dodging
  :phase :tactics :priority 1300
  :when (let ((target (getf strategy :tactics-target)))
          (and target (eql (ship-last-tactic target) #\D)))
  :action (let* ((ship (getf strategy :tactics-ship))
                 (target (getf strategy :tactics-target)))
            (format t "[LISP] Enemy dodging - attacking with +3/+4 drive~%")
            (crt-attack-alloc (ship-pd ship) (ship-phasic ship) (ship-shield ship)
                              (or (ship-last-drive target) 0) :counter-dodge))
  :doc "Enemy dodging -> counter-dodge attack.")

(define-strategy-rule tactics-enemy-attacking
  :phase :tactics :priority 1400
  :when (let ((target (getf strategy :tactics-target)))
          (and target (eql (ship-last-tactic target) #\A)))
  :action (let ((ship (getf strategy :tactics-ship)))
            (format t "[LISP] Enemy attacking - dodging to counter~%")
            (crt-dodge-alloc (ship-pd ship) (ship-phasic ship) (ship-shield ship)))
  :doc "Enemy attacking -> dodge to counter.")

(define-strategy-rule tactics-pd-advantage
  :phase :tactics :priority 1500
  :when (let* ((ship (getf strategy :tactics-ship))
               (target (getf strategy :tactics-target)))
          (and target
               (> (ship-pd ship)
                  (* (theta 'theta-combat-advantage-ratio)
                     (or (ship-pd target)
                         (theta 'theta-enemy-pd-default))))))
  :action (let* ((ship (getf strategy :tactics-ship))
                 (target (getf strategy :tactics-target)))
            (format t "[LISP] PD advantage - attacking~%")
            (crt-attack-alloc (ship-pd ship) (ship-phasic ship) (ship-shield ship)
                              (or (ship-pd target)
                                  (theta 'theta-enemy-pd-default))
                              :standard))
  :doc "PD advantage -> attack.")

(define-strategy-rule tactics-default-dodge
  :phase :tactics :priority 1600
  :when t
  :action (let ((ship (getf strategy :tactics-ship)))
            (format t "[LISP] Matched strength - dodging~%")
            (crt-dodge-alloc (ship-pd ship) (ship-phasic ship) (ship-shield ship)))
  :doc "Default: dodge.")

;;; ============================================================================
;;; Pick/Drop Rules (from decide-pickdrop-phase, first-match)
;;; ============================================================================

(define-strategy-rule pickdrop-drop
  :phase :pickdrop :priority 100
  :when (find-drop-opportunity slate)
  :action (let ((cmd (find-drop-opportunity slate)))
            (format t "[LISP] -> issuing DROP~%")
            (list cmd))
  :doc "Drop systemship at enemy base.")

(define-strategy-rule pickdrop-pick
  :phase :pickdrop :priority 200
  :when (find-pick-opportunity slate)
  :action (let ((cmd (find-pick-opportunity slate)))
            (format t "[LISP] -> issuing PICK~%")
            (list cmd))
  :doc "Pick up systemship.")

(define-strategy-rule pickdrop-done
  :phase :pickdrop :priority 300
  :when t
  :action (progn
            (format t "[LISP] -> NEXT (no pick/drop opportunities)~%")
            (list (cmd-next)))
  :doc "No pick/drop opportunities.")
