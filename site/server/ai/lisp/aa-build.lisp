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

(defun decide-build-phase (slate)
  "Decide ONE action in build phase. Called repeatedly until NEXT.
   State machine:
   1. Draft with specs (pd>0) -> commit it (bc)
   2. Draft without specs -> set specs (bs)
   3. Ship not deployed -> deploy it (ds)
   4. Should repair? -> repair command
   5. Should build? -> create draft (bn)
   6. Otherwise -> advance (NEXT)"
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
              (spec (choose-ship-design slate)))
         (format t "[LISP] -> bs ~A ~A~%" name spec)
         (list (make-cmd "bs" (format nil "~A ~A" name spec)))))

      ;; Ship needs deployment
      ((ship-needs-deploy-p ships)
       (let ((ship (find-undeployed-ship ships)))
         (format t "[LISP] -> ds ~A~%" (ship-name ship))
         (list (make-cmd "ds" (format nil "~A astrex" (ship-code ship))))))

      ;; Check for repairs needed
      ((should-repair-p slate)
       (issue-repair-command slate))

      ;; Should we build a base defender (systemship)?
      ((should-build-defender-p slate)
       (let ((name (next-ship-name)))
         (format t "[LISP] -> bn s ~A (base defender)~%" name)
         (list (make-cmd "bn" (format nil "s ~A" name)))))

      ;; Should we start building a new warpship?
      ((should-build-p slate)
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

(defun choose-ship-design (slate)
  "Choose ship design based on game state and strategy.
   Consider: tech level, enemy composition, fleet balance, systemship flag."
  (let* ((round (slate-round slate))
         (tech (slate-tech-level slate))
         (own-ships (slate-own-ships slate))
         (enemy-ships (slate-enemy-ships slate))
         (drafts (slate-drafts slate))
         (num-brawlers (count-ship-type own-ships :brawler))
         (num-interceptors (count-ship-type own-ships :interceptor)))

    ;; Check if the draft is a systemship (no WG)
    (when (and drafts (is-draft-systemship-p (first drafts)))
      (format t "[LISP] Designing systemship defender~%")
      (return-from choose-ship-design *defender-spec*))

    (cond
      ;; First ship should be a brawler (main combat)
      ((null own-ships)
       (format t "[LISP] First ship - building brawler~%")
       *brawler-spec*)

      ;; If tech 1+ and we have ships, build stronger
      ((and (>= tech 1) (< num-brawlers 3))
       (format t "[LISP] Tech ~A - building brawler~%" tech)
       *brawler-spec*)

      ;; Need interceptors to spread across bases
      ((< num-interceptors 2)
       (format t "[LISP] Fleet needs interceptors~%")
       *interceptor-spec*)

      ;; Default: brawler
      (t *brawler-spec*))))

(defun is-draft-systemship-p (draft)
  "Check if draft is a systemship (no warp generator)."
  (not (ship-warpship-p draft)))

(defun count-ship-type (ships type)
  "Count ships of approximate type based on stats."
  (count-if
   (lambda (s)
     (let ((pd (ship-pd s))
           (beam (ship-beam s)))
       (case type
         (:brawler (and (>= pd 5) (>= beam 3)))
         (:interceptor (and (>= pd 4) (< beam 3)))
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

(defun should-build-defender-p (slate)
  "Should we build a systemship base defender?
   Yes if: bases are threatened AND no defender at base AND can afford.
   Systemships save 5 BP (no warp gen) but can't move or retreat."
  (let* ((credits (slate-credits slate))
         (own-bases (slate-own-bases slate))
         (own-ships (slate-own-ships slate))
         (defender-cost 13)  ; Light defender cost
         (defenders-at-base (count-ships-at-bases own-ships own-bases)))

    ;; Build defender if:
    ;; - Have at least 2 bases
    ;; - No defender at base
    ;; - Can afford
    ;; - Already have at least 1 warpship
    (and (>= (length own-bases) 2)
         (< defenders-at-base 1)
         (>= (length own-ships) 1)
         (>= credits (+ defender-cost *minimum-reserve*)))))

(defun count-ships-at-bases (ships bases)
  "Count how many ships are stationed at our bases."
  (count-if (lambda (s)
              (member (ship-hex s) bases :test #'string=))
            ships))

(defun should-build-p (slate)
  "Heuristic: Should we build another warpship?
   Consider: credits, fleet size, enemy strength, tech timing."
  (let* ((credits (slate-credits slate))
         (our-ships (length (slate-own-ships slate)))
         (enemy-ships (length (slate-enemy-ships slate)))
         (ship-cost 17)  ; Brawler cost
         (need-ships (max 2 (+ enemy-ships 1))))

    ;; Don't build if waiting for tech increase
    (when (should-wait-for-tech-p slate)
      (format t "[LISP] Waiting for tech level increase~%")
      (return-from should-build-p nil))

    ;; Build if: affordable AND need more ships
    (and (>= credits (+ ship-cost *minimum-reserve*))
         (< our-ships need-ships)
         (< our-ships 5)))) ; Cap at 5 ships for manageability

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
  "Check if ship has taken damage (simplified check)."
  ;; A damaged ship would have reduced PD from original
  ;; For now, assume damage if PD < 5 (typical starting PD)
  (< (ship-pd ship) 5))

(defun issue-repair-command (slate)
  "Issue repair for first damaged ship at base."
  (let* ((bases (slate-own-bases slate))
         (ships (slate-own-ships slate))
         (damaged (find-if (lambda (s)
                            (and (member (ship-hex s) bases :test #'string=)
                                 (ship-damaged-p s)))
                          ships)))
    (if damaged
        (let ((repair-pd (min 3 (- 6 (ship-pd damaged))))) ; Repair up to PD=6
          (format t "[LISP] -> rp ~A pd=~A~%" (ship-name damaged) repair-pd)
          (list (make-cmd "rp" (format nil "~A pd=~A" (ship-code damaged) repair-pd))))
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
