;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-build.lisp - Build Phase Decision Logic (Strategic)

;;; Build Point Costs:
;;;   WG = 5 BP (required for warpship)
;;;   PD, B, S, T, SR = 1 BP each
;;;   M = 3 missiles per 1 BP
;;;
;;; Tech Level Timing (Advanced Scenario):
;;;   Turns 1-4:  Tech 0
;;;   Turns 5-8:  Tech 1
;;;   Turns 9-12: Tech 2
;;;   etc.
;;;
;;; Tech adds to: Beam damage, Missile damage, Screen absorption

(defparameter *ship-name-counter* 0)

(defun next-ship-name ()
  "Generate next ship name (lowercase)."
  (incf *ship-name-counter*)
  (format nil "alpha~D" *ship-name-counter*))

;;; ----------------------------------------------------------------------------
;;; Ship Design Templates
;;; ----------------------------------------------------------------------------

;;; Ship Design Templates (defined in aa-theta.lisp *ship-templates*)
;;; Access via (get-ship-template :brawler), (get-ship-template :defender), etc.

;;; ----------------------------------------------------------------------------
;;; Build Phase Entry
;;; ----------------------------------------------------------------------------

;;; Minimum reserve now in theta: (theta 'theta-minimum-reserve)

(defun decide-build-phase (slate &optional strategy)
  "Decide ONE action in build phase. Called repeatedly until NEXT.
   STRATEGY is the full strategic state plist from compute-strategic-state.
   Gen4: dispatches to build rules via fire-first-producing-rule."
  (format t "[LISP] decide-build: drafts=~A ships=~A credits=~A round=~A tech=~A~%"
          (length (slate-drafts slate)) (length (slate-own-ships slate))
          (slate-credits slate) (slate-round slate) (slate-tech-level slate))
  (fire-first-producing-rule :build slate strategy))

;;; ----------------------------------------------------------------------------
;;; Strategic Ship Design Selection
;;; ----------------------------------------------------------------------------

(defun choose-ship-design (slate &optional strategy)
  "Choose ship design based on game state and strategy.
   Gen4: dispatches to design rules via fire-first-matching-rule."
  (fire-first-matching-rule :design slate strategy))

(defun enemy-heavy-screens-p (enemy-ships)
  "Check if any enemy ship has heavy screens (S >= 4)."
  (some (lambda (s) (>= (ship-screen s) (theta 'theta-heavy-screen-threshold)))
        enemy-ships))

(defun is-draft-systemship-p (draft)
  "Check if draft is a systemship (no warp generator)."
  (not (ship-warpship-p draft)))

(defun count-ship-type (ships type)
  "Count ships of approximate type based on stats."
  (count-if
   (lambda (s)
     (let ((pd (ship-pd s))
           (beam (ship-beam s))
           (tube (ship-tube s)))
       (case type
         (:brawler (and (>= pd 5) (>= beam 3)))
         (:interceptor (and (>= pd 4) (< beam 3) (= tube 0)))
         (:missile-boat (> tube 0))
         (t nil))))
   ships))

;;; ----------------------------------------------------------------------------
;;; Tech Level Timing
;;; ----------------------------------------------------------------------------

(defun turns-until-tech-up (slate)
  "Calculate turns until next tech level increase.
   Tech increases every 4 turns: 1-4=T0, 5-8=T1, 9-12=T2, etc."
  (let* ((round (slate-round slate))
         (tech-boundary (* (1+ (floor round 4)) 4)))
    (- tech-boundary round)))

(defun should-wait-for-tech-p (slate)
  "Should we save BP for higher tech ships?
   Yes if: 1-2 turns from tech increase AND have reasonable fleet."
  (let ((turns (turns-until-tech-up slate))
        (num-ships (length (slate-own-ships slate)))
        (credits (slate-credits slate)))
    (and (<= turns (theta 'theta-wait-tech-turns))
         (>= num-ships (theta 'theta-wait-tech-ships-min))
         (>= credits (theta 'theta-wait-tech-credits-min)))))

;;; ----------------------------------------------------------------------------
;;; Build Decision
;;; ----------------------------------------------------------------------------

(defun should-build-defender-p (slate &optional strategy)
  "Should we build a systemship base defender?
   Consults strategy :build-type when available.
   Capped: at most 1 defender per unguarded base, max 2 total defenders."
  (let* ((credits (slate-credits slate))
         (own-bases (slate-own-bases slate))
         (own-ships (slate-own-ships slate))
         (defender-cost (theta 'theta-defender-cost))
         (build-type (when strategy (getf strategy :build-type)))
         (unguarded (when strategy (getf strategy :unguarded-own-bases)))
         (defenders-at-base (count-ships-at-bases own-ships own-bases))
         (existing-defenders (count-if-not #'ship-warpship-p own-ships))
         (bases-needing (if unguarded (length unguarded) 0)))

    ;; Strategy-driven: build defender if strategy says so AND under cap
    (when (and build-type (eq build-type :systemship-defender))
      (return-from should-build-defender-p
        (and (> bases-needing 0)
             (< existing-defenders bases-needing)
             (<= existing-defenders (theta 'theta-max-defenders))
             (>= credits (+ defender-cost (theta 'theta-minimum-reserve))))))

    ;; Fallback: original logic
    (and (>= (length own-bases) (theta 'theta-defender-base-threshold))
         (< defenders-at-base 1)
         (>= (length own-ships) 1)
         (<= existing-defenders (theta 'theta-max-defenders))
         (>= credits (+ defender-cost (theta 'theta-minimum-reserve))))))

(defun count-ships-at-bases (ships bases)
  "Count how many ships are stationed at our bases."
  (count-if (lambda (s)
              (member (ship-hex s) bases :test #'string=))
            ships))

(defun should-build-p (slate &optional strategy)
  "Heuristic: Should we build another warpship?
   Strategy-driven: no hardcoded cap. Build until fleet meets strategic need."
  (let* ((credits (slate-credits slate))
         (ship-cost (theta 'theta-warpship-cost))
         (ships-to-build (when strategy (getf strategy :ships-to-build)))
         (save-for-tech (when strategy (getf strategy :save-for-tech-p)))
         (warp-count (if strategy
                         (getf strategy :warpship-count)
                         (count-if #'ship-warpship-p (slate-own-ships slate))))
         (need-ships (if ships-to-build
                         (+ warp-count ships-to-build)
                         (max 2 (1+ (length (slate-enemy-ships slate)))))))

    ;; Don't build if strategy says save for tech
    (when (or save-for-tech (should-wait-for-tech-p slate))
      (format t "[LISP] Waiting for tech level increase~%")
      (return-from should-build-p nil))

    ;; Build if: affordable AND need more ships (no hardcoded cap)
    (and (>= credits (+ ship-cost (theta 'theta-minimum-reserve)))
         (< warp-count need-ships))))

;;; ----------------------------------------------------------------------------
;;; Repair Logic
;;; ----------------------------------------------------------------------------

(defun should-repair-p (slate)
  "Check if any ship at base needs repair."
  (let ((bases (slate-own-bases slate))
        (ships (slate-own-ships slate)))
    (some (lambda (ship)
            (and (member (ship-hex ship) bases :test #'string=)
                 (ship-damaged-p ship)))
          ships)))

(defun ship-damaged-p (ship)
  "Check if any attribute is below max."
  (or (< (ship-pd ship) (ship-pd-max ship))
      (< (ship-beam ship) (ship-beam-max ship))
      (< (ship-screen ship) (ship-screen-max ship))
      (< (ship-tube ship) (ship-tube-max ship))))

(defun find-most-damaged-attr (ship)
  "Find the most damaged attribute and how much to repair.
   Returns (attr-name repair-amount) or NIL."
  (let ((best-attr nil)
        (best-diff 0))
    ;; Check PD
    (let ((diff (- (ship-pd-max ship) (ship-pd ship))))
      (when (> diff best-diff)
        (setf best-attr "pd" best-diff diff)))
    ;; Check beam
    (let ((diff (- (ship-beam-max ship) (ship-beam ship))))
      (when (> diff best-diff)
        (setf best-attr "b" best-diff diff)))
    ;; Check screen
    (let ((diff (- (ship-screen-max ship) (ship-screen ship))))
      (when (> diff best-diff)
        (setf best-attr "s" best-diff diff)))
    ;; Check tube
    (let ((diff (- (ship-tube-max ship) (ship-tube ship))))
      (when (> diff best-diff)
        (setf best-attr "t" best-diff diff)))
    (when best-attr
      (list best-attr (min 3 best-diff)))))

(defun issue-repair-command (slate)
  "Issue repair for first damaged ship at base."
  (let* ((bases (slate-own-bases slate))
         (ships (slate-own-ships slate))
         (damaged (find-if (lambda (s)
                            (and (member (ship-hex s) bases :test #'string=)
                                 (ship-damaged-p s)))
                          ships)))
    (if damaged
        (let ((repair-info (find-most-damaged-attr damaged)))
          (if repair-info
              (let ((attr (first repair-info))
                    (amount (second repair-info)))
                (format t "[LISP] -> rp ~A ~A=~A~%" (ship-name damaged) attr amount)
                (list (make-cmd "rp" (format nil "~A ~A=~A" (ship-code damaged) attr amount))))
              (list (cmd-next))))
        (list (cmd-next)))))

;;; ----------------------------------------------------------------------------
;;; State Predicates
;;; ----------------------------------------------------------------------------

(defun draft-ready-p (drafts)
  "Check if first draft has specs and is ready to commit."
  (and drafts
       (let ((d (first drafts)))
         (> (ship-pd d) 0))))

(defun draft-needs-specs-p (drafts)
  "Check if first draft exists but has no specs."
  (and drafts
       (let ((d (first drafts)))
         (<= (ship-pd d) 0))))

(defun ship-needs-deploy-p (ships)
  "Check if any ship needs deployment."
  (find-undeployed-ship ships))

(defun find-undeployed-ship (ships)
  "Find a ship with no hex (not deployed)."
  (find-if (lambda (s)
             (let ((hex (ship-hex s)))
               (or (null hex) (string= hex ""))))
           ships))

(defun choose-deploy-base (slate ships &optional strategy)
  "Choose which base to deploy a ship at. Spread defenders across bases.
   Prefers bases with fewest ships, then unguarded bases from strategy."
  (let ((own-bases (slate-own-bases slate))
        (best-base nil)
        (best-count 999))
    ;; Find base with fewest deployed ships
    (dolist (base own-bases)
      (let ((count (count-if (lambda (s)
                               (string-equal (ship-hex s) base))
                             ships)))
        (when (< count best-count)
          (setf best-count count)
          (setf best-base base))))
    (or best-base (first-base slate))))
