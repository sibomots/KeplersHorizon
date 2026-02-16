;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-goals.lisp - Goal Evaluation Engine (Ontology K35/K37-K40)
;;;;
;;;; Implements the MPC (model-predictive control) goal evaluation
;;;; for the AI agent. Each MSS cycle:
;;;;   1. GATHER reads :world state from DB
;;;;   2. CALCULATE evaluates :goal predicates against :world
;;;;   3. RENDER executes the single best action
;;;;   4. Next GATHER closes the feedback loop
;;;;
;;;; Every economic action must trace to an active goal.
;;;; Every goal must trace to :win-game via supports chains.
;;;; No action fires without purpose. No goal loops.

;;; ----------------------------------------------------------------------------
;;; Goal Structure
;;; ----------------------------------------------------------------------------
;;; A goal is a plist:
;;;   :objective    — keyword naming the objective
;;;   :priority     — integer, lower = higher priority
;;;   :satisfied    — T if objective is currently met
;;;   :relevant     — T if objective is actionable in current state
;;;   :needed-resources — list of resource-type strings this goal consumes
;;;   :params       — optional parameters (e.g., resource type, qty)

;;; ----------------------------------------------------------------------------
;;; Missile Recipe Constants (from K31)
;;; ----------------------------------------------------------------------------

(defparameter *missile-recipe*
  '(("ferrous" . 2) ("radioactive" . 1) ("volatile" . 1))
  "Missile fabrication recipe: resource-type → quantity needed.")

(defparameter *missile-recipe-resources*
  '("ferrous" "radioactive" "volatile")
  "Resource types consumed by missile fabrication.")

;;; ----------------------------------------------------------------------------
;;; Fleet Aggregate Queries
;;; ----------------------------------------------------------------------------

(defun fleet-resource-total (slate resource-type)
  "Sum of a resource type across all ships in fleet."
  (let ((total 0))
    (dolist (ship (slate-own-ships slate))
      (incf total
            (cond
              ((string-equal resource-type "ferrous") (ship-cargo-ferrous ship))
              ((string-equal resource-type "rare_earth") (ship-cargo-rare-earth ship))
              ((string-equal resource-type "radioactive") (ship-cargo-radioactive ship))
              ((string-equal resource-type "crystalline") (ship-cargo-crystalline ship))
              ((string-equal resource-type "volatile") (ship-cargo-volatile ship))
              ((string-equal resource-type "water") (ship-cargo-water ship))
              ((string-equal resource-type "organic") (ship-cargo-organic ship))
              ((string-equal resource-type "exotic") (ship-cargo-exotic ship))
              (t 0))))
    total))

(defun fleet-missile-ratio (slate)
  "Ratio of current missiles to max capacity across fleet.
   Returns 1.0 if fleet has no missile capacity."
  (let ((total-missiles 0)
        (total-max 0))
    (dolist (ship (slate-own-ships slate))
      (incf total-missiles (ship-missile ship))
      (incf total-max (ship-missiles-max ship)))
    (if (> total-max 0)
        (/ (float total-missiles) total-max)
        1.0)))

;;; ----------------------------------------------------------------------------
;;; Objective Predicates: satisfied-p
;;; ----------------------------------------------------------------------------

(defun objective-satisfied-p (objective slate)
  "Is this objective currently satisfied?"
  (case objective
    (:maintain-missile-stock
     (>= (fleet-missile-ratio slate) 0.5))

    (:fabricate-materiel
     ;; satisfied when we've just fabricated or have enough missiles
     (objective-satisfied-p :maintain-missile-stock slate))

    (:acquire-resource
     ;; satisfied when fleet has all ingredients for missile recipe
     (null (missile-ingredients-needed slate)))

    (:survey-for-yield
     ;; satisfied when no ship is at a system with knowledge < surveyed
     ;; that also has extractable resources
     (null (find-surveyable-for-goal slate)))

    (:sell-excess-cargo
     ;; satisfied when no ship has excess cargo (cargo serving no goal)
     (null (find-excess-cargo-ship slate)))

    (:resupply-missiles
     ;; satisfied when no ship at base needs missile resupply
     (null (find-resupplyable-ship-for-goal slate)))

    (:salvage-opportunity
     ;; satisfied when ship is at salvageable hex
     (null (find-salvage-opportunity slate)))

    (otherwise t)))

;;; ----------------------------------------------------------------------------
;;; Objective Predicates: relevant-p
;;; ----------------------------------------------------------------------------

(defun objective-relevant-p (objective slate)
  "Is this objective relevant (actionable) in the current state?
   A goal is relevant when its preconditions are met AND
   pursuing it would advance the supports chain toward :win-game."
  (case objective
    (:maintain-missile-stock
     ;; always relevant — top of economic chain
     t)

    (:fabricate-materiel
     ;; relevant only when missile stock is unsatisfied
     ;; AND a ship at a shipyard has the ingredients
     (and (not (objective-satisfied-p :maintain-missile-stock slate))
          (find-fabricatable-ship-for-goal slate)))

    (:acquire-resource
     ;; relevant only when fabrication needs ingredients
     ;; AND a ship can extract or buy
     (and (not (objective-satisfied-p :acquire-resource slate))
          (not (objective-satisfied-p :maintain-missile-stock slate))
          (or (find-extractable-for-goal slate)
              (find-buyable-for-goal slate))))

    (:survey-for-yield
     ;; relevant only when acquiring resources
     ;; AND ship is at a system with knowledge < surveyed
     (and (not (objective-satisfied-p :acquire-resource slate))
          (not (objective-satisfied-p :maintain-missile-stock slate))
          (find-surveyable-for-goal slate)))

    (:sell-excess-cargo
     ;; relevant only when cargo serves NO active goal
     ;; AND ship is at trade hub
     (find-excess-cargo-ship slate))

    (:resupply-missiles
     ;; relevant when ship at own base has missiles < 50% of max
     (and (not (objective-satisfied-p :maintain-missile-stock slate))
          (find-resupplyable-ship-for-goal slate)))

    (:salvage-opportunity
     ;; relevant when ship is at salvageable hex
     (find-salvage-opportunity slate))

    (otherwise nil)))

;;; ----------------------------------------------------------------------------
;;; Goal Evaluation: MPC Core
;;; ----------------------------------------------------------------------------

(defun evaluate-goals (slate)
  "Evaluate all economic objectives against current world state.
   Returns list of goal plists sorted by priority.
   This is the CALCULATE step of the MPC loop."
  (let* ((needed (missile-ingredients-needed slate))
         (needed-types (mapcar #'car needed))
         (goals
           (list
             (list :objective :maintain-missile-stock
                   :priority 1
                   :satisfied (objective-satisfied-p :maintain-missile-stock slate)
                   :relevant (objective-relevant-p :maintain-missile-stock slate)
                   :needed-resources nil)

             (list :objective :resupply-missiles
                   :priority 2
                   :satisfied (objective-satisfied-p :resupply-missiles slate)
                   :relevant (objective-relevant-p :resupply-missiles slate)
                   :needed-resources nil)

             (list :objective :fabricate-materiel
                   :priority 3
                   :satisfied (objective-satisfied-p :fabricate-materiel slate)
                   :relevant (objective-relevant-p :fabricate-materiel slate)
                   :needed-resources *missile-recipe-resources*)

             (list :objective :acquire-resource
                   :priority 4
                   :satisfied (objective-satisfied-p :acquire-resource slate)
                   :relevant (objective-relevant-p :acquire-resource slate)
                   :needed-resources needed-types)

             (list :objective :survey-for-yield
                   :priority 5
                   :satisfied (objective-satisfied-p :survey-for-yield slate)
                   :relevant (objective-relevant-p :survey-for-yield slate)
                   :needed-resources nil)

             (list :objective :salvage-opportunity
                   :priority 6
                   :satisfied (objective-satisfied-p :salvage-opportunity slate)
                   :relevant (objective-relevant-p :salvage-opportunity slate)
                   :needed-resources nil)

             (list :objective :sell-excess-cargo
                   :priority 8
                   :satisfied (objective-satisfied-p :sell-excess-cargo slate)
                   :relevant (objective-relevant-p :sell-excess-cargo slate)
                   :needed-resources nil))))
    ;; Log goal state
    (dolist (g goals)
      (format t "[LISP] Goal ~A: sat=~A rel=~A pri=~A~%"
              (getf g :objective)
              (getf g :satisfied)
              (getf g :relevant)
              (getf g :priority)))
    goals))

(defun find-actionable-goal (goals)
  "Find highest-priority goal that is unsatisfied AND relevant.
   Returns goal plist or NIL."
  (dolist (g goals)
    (when (and (not (getf g :satisfied))
               (getf g :relevant))
      (return-from find-actionable-goal g)))
  nil)

;;; ----------------------------------------------------------------------------
;;; Cargo-Serves-Active-Goal Predicate
;;; ----------------------------------------------------------------------------

(defun cargo-serves-active-goal-p (resource-type goals)
  "Does any active (unsatisfied + relevant) goal need this resource type?
   If yes, do NOT sell this cargo."
  (some (lambda (g)
          (and (not (getf g :satisfied))
               (getf g :relevant)
               (member resource-type (getf g :needed-resources)
                       :test #'string-equal)))
        goals))

;;; ----------------------------------------------------------------------------
;;; Ingredient Queries
;;; ----------------------------------------------------------------------------

(defun missile-ingredients-needed (slate)
  "Return alist of (resource-type . qty-short) for missile recipe.
   Only includes resources where fleet has less than recipe requires."
  (let ((needs nil))
    (dolist (ingredient *missile-recipe*)
      (let* ((rtype (car ingredient))
             (qty-required (cdr ingredient))
             (qty-have (fleet-resource-total slate rtype))
             (deficit (- qty-required qty-have)))
        (when (> deficit 0)
          (push (cons rtype deficit) needs))))
    (nreverse needs)))

;;; ----------------------------------------------------------------------------
;;; Goal-Specific Ship Finders (used by relevant-p and goal-to-action)
;;; ----------------------------------------------------------------------------

(defun find-surveyable-for-goal (slate)
  "Find ship at system with knowledge < surveyed AND resources exist there.
   Only survey when it serves the acquire-resource chain."
  (dolist (ship (slate-own-ships slate))
    (let ((sys (ship-at-system ship)))
      (when (and (not (string= sys ""))
                 (< (knowledge-level-value (get-knowledge-level slate sys)) 2)
                 (resources-at-system slate sys))
        (return-from find-surveyable-for-goal ship))))
  nil)

(defun find-extractable-for-goal (slate)
  "Find ship that can extract a NEEDED resource.
   Only extract what the active goal chain requires."
  (let ((needed (missile-ingredients-needed slate)))
    (when needed
      (dolist (ship (slate-own-ships slate))
        (let ((sys (ship-at-system ship)))
          (when (and (not (string= sys ""))
                     (> (ship-cargo-free-space ship) 0))
            ;; Check if system has a resource we need
            (let ((sys-resources (resources-at-system slate sys)))
              (dolist (ingredient needed)
                (let ((rtype (car ingredient)))
                  (when (find rtype sys-resources
                              :key #'resource-type :test #'string-equal)
                    (return-from find-extractable-for-goal
                      (list ship rtype))))))))))))

(defun find-buyable-for-goal (slate)
  "Find ship at trade hub that can buy a NEEDED resource."
  (let ((needed (missile-ingredients-needed slate)))
    (when (and needed (> (slate-credits slate) 50))
      (dolist (ship (slate-own-ships slate))
        (let ((sys (ship-at-system ship)))
          (when (and (not (string= sys ""))
                     (can-trade-at-p slate sys)
                     (> (ship-cargo-free-space ship) 0))
            (return-from find-buyable-for-goal
              (list ship (caar needed)))))))))

(defun find-fabricatable-ship-for-goal (slate)
  "Find ship at shipyard with all missile ingredients."
  (let ((player (slate-get slate :aa-player)))
    (dolist (ship (slate-own-ships slate))
      (let ((sys (ship-at-system ship)))
        (when (and (not (string= sys ""))
                   (can-fabricate-at-p slate sys player)
                   (>= (ship-cargo-ferrous ship) 2)
                   (>= (ship-cargo-radioactive ship) 1)
                   (>= (ship-cargo-volatile ship) 1))
          (return-from find-fabricatable-ship-for-goal ship)))))
  nil)

(defun find-excess-cargo-ship (slate)
  "Find ship at trade hub with cargo that serves no active goal.
   Evaluates cargo against active goals to determine excess."
  (let ((goals (list
                 (list :objective :fabricate-materiel
                       :satisfied (objective-satisfied-p :fabricate-materiel slate)
                       :relevant t
                       :needed-resources *missile-recipe-resources*)
                 (list :objective :acquire-resource
                       :satisfied (objective-satisfied-p :acquire-resource slate)
                       :relevant t
                       :needed-resources (mapcar #'car (missile-ingredients-needed slate))))))
    (dolist (ship (slate-own-ships slate))
      (let ((sys (ship-at-system ship)))
        (when (and (not (string= sys ""))
                   (can-trade-at-p slate sys)
                   (> (ship-cargo-total ship) 0))
          ;; Check if ANY cargo on this ship is excess
          (let ((has-excess nil))
            (flet ((check-excess (rtype qty)
                     (when (and (> qty 0)
                                (not (cargo-serves-active-goal-p rtype goals)))
                       (setf has-excess t))))
              (check-excess "ferrous" (ship-cargo-ferrous ship))
              (check-excess "rare_earth" (ship-cargo-rare-earth ship))
              (check-excess "radioactive" (ship-cargo-radioactive ship))
              (check-excess "crystalline" (ship-cargo-crystalline ship))
              (check-excess "volatile" (ship-cargo-volatile ship))
              (check-excess "water" (ship-cargo-water ship))
              (check-excess "organic" (ship-cargo-organic ship))
              (check-excess "exotic" (ship-cargo-exotic ship)))
            (when has-excess
              (return-from find-excess-cargo-ship ship)))))))
  nil)

(defun find-resupplyable-ship-for-goal (slate)
  "Find ship at own base needing missiles."
  (let ((own-bases (slate-own-bases slate)))
    (dolist (ship (slate-own-ships slate))
      (let ((hex (ship-hex ship))
            (missiles (ship-missile ship))
            (max-missiles (ship-missiles-max ship)))
        (when (and (member hex own-bases :test #'string=)
                   (> max-missiles 0)
                   (< missiles max-missiles)
                   (< (/ (float missiles) max-missiles) 0.5))
          (return-from find-resupplyable-ship-for-goal ship)))))
  nil)

(defun find-salvage-opportunity (slate)
  "Find ship at hex with salvageable objects."
  (dolist (ship (slate-own-ships slate))
    (let ((hex (ship-hex ship)))
      (when (and (not (string= hex ""))
                 (salvageables-at-hex slate hex))
        (return-from find-salvage-opportunity ship))))
  nil)
