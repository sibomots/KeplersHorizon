;;;; aa-combat.lisp - Combat Phase Decision Logic

;;; Combat tactics: A (Attack), D (Dodge), R (Retreat)
;;; Combat order format: co SHIP TACTIC TARGET PD=n B=n S=n T=n

;;; ----------------------------------------------------------------------------
;;; Combat Phase Entry
;;; ----------------------------------------------------------------------------

(defun decide-combat-phase (slate)
  "Decide what to do in combat phase.
   If in combat, issue orders for each ship. Otherwise advance."
  (if (not (in-combat-p slate))
      (list (cmd-next))
      (issue-combat-orders slate)))

;;; ----------------------------------------------------------------------------
;;; Combat Orders
;;; ----------------------------------------------------------------------------

(defun issue-combat-orders (slate)
  "Issue combat orders for all ships in contested hexes, then commit."
  (let* ((contested (slate-contested-hexes slate))
         (own-ships (slate-own-ships slate))
         (enemy-ships (slate-enemy-ships slate))
         (orders nil))

    ;; For each contested hex, issue orders for our ships there
    (dolist (hex contested)
      (let ((our-ships (ships-at-hex own-ships hex))
            (their-ships (ships-at-hex enemy-ships hex)))
        (when (and our-ships their-ships)
          (dolist (ship our-ships)
            (let ((order (make-combat-order ship (first their-ships))))
              (push order orders))))))

    ;; Add commit at the end
    (if orders
        (append (nreverse orders) (list (make-cmd "cc")))
        (list (cmd-next)))))

(defun make-combat-order (ship target)
  "Create a combat order for SHIP attacking TARGET.
   Uses Attack tactic, allocates all PD to drive and weapons."
  (let* ((pd (ship-pd ship))
         (beam (ship-beam ship))
         (screen (ship-screen ship))
         ;; Simple allocation: prioritize drive for hit chance
         (drive-alloc (min pd 3))
         (beam-alloc (min (- pd drive-alloc) beam))
         (screen-alloc (min (- pd drive-alloc beam-alloc) screen)))
    (make-cmd "co"
              (format nil "~A attack ~A PD=~D B=~D S=~D"
                      (ship-code ship)
                      (ship-code target)
                      drive-alloc
                      beam-alloc
                      screen-alloc))))

;;; ----------------------------------------------------------------------------
;;; Damage Assignment (future)
;;; ----------------------------------------------------------------------------

(defun assign-damage (slate ship hits)
  "Decide how to assign HITS damage to SHIP.
   STUB: Returns damage assignment command."
  (declare (ignore slate ship hits))
  ;; BUGBUG: Need to implement damage assignment logic
  ;; Priority: preserve PD > preserve weapons > sacrifice screens
  nil)
