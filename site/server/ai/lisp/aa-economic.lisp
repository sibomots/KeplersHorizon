;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-economic.lisp - Economic Layer Decision Logic (Goal-Driven)
;;;;
;;;; Ontology anchor: K35 (milieu goals), K27-K34 (economic rules)
;;;;
;;;; Every economic action traces to an active goal.
;;;; Every goal traces to :win-game via supports chains.
;;;; No action fires without purpose. No goal loops.
;;;;
;;;; Goal chain:
;;;;   :win-game
;;;;     ← :maintain-missile-stock (fleet missiles >= 50%)
;;;;       ← :fabricate-materiel (convert resources at shipyard)
;;;;         ← :acquire-resource (extract or buy needed ingredients)
;;;;           ← :survey-for-yield (improve knowledge for better yield)
;;;;   :sell-excess-cargo (ONE-SHOT: only when cargo serves no goal)

;;; ----------------------------------------------------------------------------
;;; Main Economic Decision Entry Point (MPC Controller)
;;; ----------------------------------------------------------------------------

(defun decide-economic-actions (slate)
  "Returns ONE economic command or NIL.
   Called from build phase after repair checks.

   MPC loop: evaluate goals → find highest priority unsatisfied →
   generate single action → return. Next GATHER closes feedback loop."
  (let* ((goals (evaluate-goals slate))
         (goal (find-actionable-goal goals)))
    (if goal
        (progn
          (format t "[LISP] Active goal: ~A (pri=~A)~%"
                  (getf goal :objective) (getf goal :priority))
          (goal-to-action goal slate goals))
        (progn
          (format t "[LISP] No active economic goals~%")
          nil))))

;;; ----------------------------------------------------------------------------
;;; Goal-to-Action Mapping
;;; ----------------------------------------------------------------------------

(defun goal-to-action (goal slate goals)
  "Map an active goal to a single command.
   Returns command list or NIL if goal cannot produce action."
  (case (getf goal :objective)
    (:maintain-missile-stock
     ;; This is the top-level check — it delegates downward.
     ;; If we reach here, something below should handle it.
     ;; Check resupply first (cheapest path to missiles).
     nil)

    (:resupply-missiles
     (issue-resupply-action slate))

    (:fabricate-materiel
     (issue-fabricate-action slate))

    (:acquire-resource
     (issue-acquire-action slate))

    (:survey-for-yield
     (issue-survey-action slate))

    (:sell-excess-cargo
     (issue-sell-action slate goals))

    (:salvage-opportunity
     (issue-salvage-action slate))

    (otherwise nil)))

;;; ----------------------------------------------------------------------------
;;; Action Generators
;;; ----------------------------------------------------------------------------

(defun issue-survey-action (slate)
  "Survey system where ship is located to improve extraction yield.
   Only fires when :survey-for-yield goal is active."
  (let ((ship (find-surveyable-for-goal slate)))
    (when ship
      (let ((sys (ship-at-system ship)))
        (format t "[LISP] -> sv ~A (goal: survey-for-yield)~%" sys)
        (list (make-cmd "sv" sys))))))

(defun issue-acquire-action (slate)
  "Extract or buy a NEEDED resource.
   Only extracts resources required by the missile recipe.
   Only fires when :acquire-resource goal is active."
  ;; Prefer extraction over buying (free vs costs credits)
  (let ((extractable (find-extractable-for-goal slate)))
    (when extractable
      (let* ((ship (first extractable))
             (rtype (second extractable)))
        (format t "[LISP] -> ex ~A ~A (goal: acquire-resource)~%"
                (ship-code ship) rtype)
        (return-from issue-acquire-action
          (list (make-cmd "ex" (format nil "~A ~A"
                                        (ship-code ship) rtype)))))))
  ;; Fallback: buy at trade hub
  (let ((buyable (find-buyable-for-goal slate)))
    (when buyable
      (let* ((ship (first buyable))
             (rtype (second buyable)))
        (format t "[LISP] -> tr buy ~A 2 (goal: acquire-resource)~%" rtype)
        (list (make-cmd "tr" (format nil "buy ~A 2" rtype)))))))

(defun issue-fabricate-action (slate)
  "Fabricate missiles at shipyard.
   Only fires when :fabricate-materiel goal is active."
  (let ((ship (find-fabricatable-ship-for-goal slate)))
    (when ship
      (format t "[LISP] -> fab missiles (goal: fabricate-materiel)~%")
      (list (make-cmd "fab" "missiles")))))

(defun issue-sell-action (slate goals)
  "Sell cargo that serves no active goal.
   ONE-SHOT: only fires when :sell-excess-cargo is active.
   Only sells cargo types not needed by any active goal."
  (let ((ship (find-excess-cargo-ship slate)))
    (when ship
      (multiple-value-bind (cargo-type qty)
          (find-best-excess-cargo slate ship goals)
        (when cargo-type
          (format t "[LISP] -> tr sell ~A ~A (goal: sell-excess-cargo)~%"
                  cargo-type qty)
          (list (make-cmd "tr" (format nil "sell ~A ~A"
                                        cargo-type qty))))))))

(defun issue-resupply-action (slate)
  "Resupply missiles at own base.
   Only fires when :resupply-missiles goal is active."
  (let ((ship (find-resupplyable-ship-for-goal slate)))
    (when ship
      (let* ((max-missiles (ship-missiles-max ship))
             (current (ship-missile ship))
             (need (- max-missiles current))
             (qty (min need 6)))
        (format t "[LISP] -> rs ~A ~A (goal: resupply-missiles)~%"
                (ship-code ship) qty)
        (list (make-cmd "rs" (format nil "~A ~A"
                                      (ship-code ship) qty)))))))

(defun issue-salvage-action (slate)
  "Salvage objects at current hex.
   Only fires when :salvage-opportunity goal is active."
  (let ((ship (find-salvage-opportunity slate)))
    (when ship
      (format t "[LISP] -> jk ~A (goal: salvage-opportunity)~%"
              (ship-code ship))
      (list (make-cmd "jk" (ship-code ship))))))

;;; ----------------------------------------------------------------------------
;;; Sell Helper: find excess cargo (not needed by active goals)
;;; ----------------------------------------------------------------------------

(defun find-best-excess-cargo (slate ship goals)
  "Find cargo type with highest value that serves NO active goal.
   Returns (values cargo-type qty) or (values NIL 0)."
  (let ((best-type nil)
        (best-qty 0)
        (best-value 0))
    (flet ((check-cargo (type-name qty)
             (when (and (> qty 0)
                        (not (cargo-serves-active-goal-p type-name goals))
                        (cargo-price-floor-ok-p slate type-name))
               (let ((value (* qty (cargo-current-price slate type-name))))
                 (when (> value best-value)
                   (setf best-type type-name
                         best-qty qty
                         best-value value))))))
      (check-cargo "ferrous" (ship-cargo-ferrous ship))
      (check-cargo "rare_earth" (ship-cargo-rare-earth ship))
      (check-cargo "radioactive" (ship-cargo-radioactive ship))
      (check-cargo "crystalline" (ship-cargo-crystalline ship))
      (check-cargo "volatile" (ship-cargo-volatile ship))
      (check-cargo "water" (ship-cargo-water ship))
      (check-cargo "organic" (ship-cargo-organic ship))
      (check-cargo "exotic" (ship-cargo-exotic ship)))
    (values best-type best-qty)))

;;; ----------------------------------------------------------------------------
;;; Market Price Helpers (kept from original)
;;; ----------------------------------------------------------------------------

(defun cargo-price-floor-ok-p (slate cargo-type)
  "Check if current price >= 80% of base price. Returns T if no price data."
  (let ((mp (get-market-price slate cargo-type)))
    (if mp
        (>= (market-price-current mp)
            (* 0.8 (market-price-base mp)))
        t)))

(defun cargo-current-price (slate cargo-type)
  "Get current market price for cargo type. Returns 1 if unknown."
  (let ((mp (get-market-price slate cargo-type)))
    (if mp (market-price-current mp) 1)))
