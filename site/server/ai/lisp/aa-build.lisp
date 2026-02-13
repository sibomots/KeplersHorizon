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

;;; The Brawler: High PD, strong beam, good screen (17-19 BP warpship)
(defparameter *brawler-spec* "PD=6 B=4 S=3 T=0 M=0"
  "Main battle ship: WG(5) + stats(12) = 17 BP")

;;; The Interceptor: Fast, light weapons (12 BP warpship)
(defparameter *interceptor-spec* "PD=5 B=2 S=2 T=0 M=0"
  "Fast pursuit ship: WG(5) + stats(9) = 14 BP")

;;; The Missile Boat: Tubes and missiles (15 BP warpship)
(defparameter *missile-boat-spec* "PD=4 B=0 S=1 T=2 M=6"
  "Alpha strike ship: WG(5) + stats(10) = 15 BP")

;;; The Fortress: Heavy systemship for base defense (no WG saves 5 BP)
(defparameter *fortress-spec* "PD=8 B=6 S=5 T=0 M=0"
  "Base defender: stats(19) = 19 BP, no warp generator")

;;; The Light Defender: Economical systemship (12 BP)
(defparameter *defender-spec* "PD=6 B=4 S=3 T=0 M=0"
  "Light base defender: stats(13) = 13 BP, no warp generator")

;;; ----------------------------------------------------------------------------
;;; Build Phase Entry
;;; ----------------------------------------------------------------------------

(defparameter *minimum-reserve* 10
  "Keep at least this many credits for emergencies/repairs.")

(defun decide-build-phase (slate &optional strategy)
  "Decide ONE action in build phase. Called repeatedly until NEXT.
   STRATEGY is the full strategic state plist from compute-strategic-state.
   State machine:
   1. Draft with specs (pd>0) -> commit it (bc)
   2. Draft without specs -> set specs (bs)
   3. Ship not deployed -> deploy it (ds)
   4. Should repair? -> repair command
   5. Economic actions? -> economic command
   6. Should build defender? -> create systemship draft (bn s)
   7. Should build warpship? -> create warpship draft (bn w)
   8. Otherwise -> advance (NEXT)"
  (let ((drafts (slate-drafts slate))
        (ships (slate-own-ships slate))
        (credits (slate-credits slate))
        (round (slate-round slate))
        (tech (slate-tech-level slate)))
    (format t "[LISP] decide-build: drafts=~A ships=~A credits=~A round=~A tech=~A~%"
            (length drafts) (length ships) credits round tech)
    (cond
      ;; Draft with specs ready to commit
      ((draft-ready-p drafts)
       (let ((name (ship-name (first drafts))))
         (format t "[LISP] -> bc ~A~%" name)
         (list (make-cmd "bc" name))))

      ;; Draft needs specs
      ((draft-needs-specs-p drafts)
       (let* ((name (ship-name (first drafts)))
              (spec (choose-ship-design slate strategy)))
         (format t "[LISP] -> bs ~A ~A~%" name spec)
         (list (make-cmd "bs" (format nil "~A ~A" name spec)))))

      ;; Ship needs deployment
      ((ship-needs-deploy-p ships)
       (let* ((ship (find-undeployed-ship ships))
              (base (ensure-hex-prefix
                     (choose-deploy-base slate ships strategy))))
         (format t "[LISP] -> ds ~A to ~A~%" (ship-name ship) base)
         (list (make-cmd "ds" (format nil "~A ~A" (ship-code ship) base)))))

      ;; Check for repairs needed
      ((should-repair-p slate)
       (issue-repair-command slate))

      ;; Economic actions before building new ships (fix C2: return value)
      ((let ((econ-cmds (decide-economic-actions slate)))
         (when econ-cmds
           (format t "[LISP] -> economic action~%")
           econ-cmds)))

      ;; Should we build a base defender (systemship)?
      ((should-build-defender-p slate strategy)
       (let ((name (next-ship-name)))
         (format t "[LISP] -> bn s ~A (base defender)~%" name)
         (list (make-cmd "bn" (format nil "s ~A" name)))))

      ;; Should we start building a new warpship?
      ((should-build-p slate strategy)
       (let ((name (next-ship-name)))
         (format t "[LISP] -> bn w ~A~%" name)
         (list (make-cmd "bn" (format nil "w ~A" name)))))

      ;; Done with build phase
      (t
       (format t "[LISP] -> NEXT~%")
       (list (cmd-next))))))

;;; ----------------------------------------------------------------------------
;;; Strategic Ship Design Selection
;;; ----------------------------------------------------------------------------

(defun choose-ship-design (slate &optional strategy)
  "Choose ship design based on game state and strategy.
   Consults strategy :design-preference when available.
   Consider: tech level, enemy composition, fleet balance, systemship flag."
  (let* ((tech (slate-tech-level slate))
         (own-ships (slate-own-ships slate))
         (enemy-ships (slate-enemy-ships slate))
         (drafts (slate-drafts slate))
         (num-brawlers (count-ship-type own-ships :brawler))
         (num-interceptors (count-ship-type own-ships :interceptor))
         (num-missile-boats (count-ship-type own-ships :missile-boat))
         (design-pref (when strategy (getf strategy :design-preference)))
         (enemy-high-screen (if strategy
                                (getf strategy :enemy-high-screen-p)
                                (enemy-heavy-screens-p enemy-ships))))

    ;; Check if the draft is a systemship (no WG)
    (when (and drafts (is-draft-systemship-p (first drafts)))
      (format t "[LISP] Designing systemship defender~%")
      (return-from choose-ship-design *defender-spec*))

    (cond
      ;; First ship should be a brawler (main combat)
      ((null own-ships)
       (format t "[LISP] First ship - building brawler~%")
       *brawler-spec*)

      ;; Strategy says missile-boat and we lack them
      ((and (eq design-pref :missile-boat) (< num-missile-boats 1))
       (format t "[LISP] Strategy: building missile boat (enemy high screen)~%")
       *missile-boat-spec*)

      ;; Enemy has heavy screens and we lack missile boats
      ((and enemy-high-screen (< num-missile-boats 1))
       (format t "[LISP] Enemy has heavy screens - building missile boat~%")
       *missile-boat-spec*)

      ;; If tech 1+ and we have ships, build stronger
      ((and (>= tech 1) (< num-brawlers 3))
       (format t "[LISP] Tech ~A - building brawler~%" tech)
       *brawler-spec*)

      ;; Need interceptors to spread across bases
      ((< num-interceptors 2)
       (format t "[LISP] Fleet needs interceptors~%")
       *interceptor-spec*)

      ;; Every 3rd+ ship could be a missile boat for variety
      ((and (>= (length own-ships) 3) (< num-missile-boats 1))
       (format t "[LISP] Adding missile boat to fleet mix~%")
       *missile-boat-spec*)

      ;; Default: brawler
      (t *brawler-spec*))))

(defun enemy-heavy-screens-p (enemy-ships)
  "Check if any enemy ship has heavy screens (S >= 4)."
  (some (lambda (s) (>= (ship-screen s) 4)) enemy-ships))

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
    (and (<= turns 2)
         (>= num-ships 2)
         (>= credits 20)))) ; Have BP to spend at higher tech

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
         (defender-cost 13)
         (build-type (when strategy (getf strategy :build-type)))
         (unguarded (when strategy (getf strategy :unguarded-own-bases)))
         (defenders-at-base (count-ships-at-bases own-ships own-bases))
         ;; Count existing systemship defenders (non-warpships)
         (existing-defenders (count-if-not #'ship-warpship-p own-ships))
         ;; How many unguarded bases actually need a defender
         (bases-needing (if unguarded (length unguarded) 0)))

    ;; Strategy-driven: build defender if strategy says so AND under cap
    (when (and build-type (eq build-type :systemship-defender))
      (return-from should-build-defender-p
        (and (> bases-needing 0)
             (< existing-defenders bases-needing)
             (<= existing-defenders 2)
             (>= credits (+ defender-cost *minimum-reserve*)))))

    ;; Fallback: original logic
    (and (>= (length own-bases) 2)
         (< defenders-at-base 1)
         (>= (length own-ships) 1)
         (<= existing-defenders 2)
         (>= credits (+ defender-cost *minimum-reserve*)))))

(defun count-ships-at-bases (ships bases)
  "Count how many ships are stationed at our bases."
  (count-if (lambda (s)
              (member (ship-hex s) bases :test #'string=))
            ships))

(defun should-build-p (slate &optional strategy)
  "Heuristic: Should we build another warpship?
   Strategy-driven: no hardcoded cap. Build until fleet meets strategic need."
  (let* ((credits (slate-credits slate))
         (ship-cost 17)
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
    (and (>= credits (+ ship-cost *minimum-reserve*))
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
