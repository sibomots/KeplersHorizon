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
;;; Torpedo Recipe Constants (from K31)
;;; ----------------------------------------------------------------------------

(defparameter *torpedo-recipe*
  '(("ferrous" . 2) ("radioactive" . 1) ("volatile" . 1))
  "Torpedo fabrication recipe: resource-type → quantity needed.")

(defparameter *torpedo-recipe-resources*
  '("ferrous" "radioactive" "volatile")
  "Resource types consumed by torpedo fabrication.")

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

(defun fleet-torpedo-ratio (slate)
  "Ratio of current torpedoes to max capacity across fleet.
   Returns 1.0 if fleet has no torpedo capacity."
  (let ((total-torpedoes 0)
        (total-max 0))
    (dolist (ship (slate-own-ships slate))
      (incf total-torpedoes (ship-torpedo ship))
      (incf total-max (ship-torpedoes-max ship)))
    (if (> total-max 0)
        (/ (float total-torpedoes) total-max)
        1.0)))

;;; ----------------------------------------------------------------------------
;;; Dynamic Variable for Goal Cross-References
;;; ----------------------------------------------------------------------------

(defvar *current-slate* nil
  "Bound during goal evaluation for cross-reference lookups.")

;;; ----------------------------------------------------------------------------
;;; Goal Declarations (Gen4 defgoal)
;;; ----------------------------------------------------------------------------
;;; Each goal traces to :win-game via :supported-by chains.
;;; :satisfied-fn and :precondition-fn are evaluated per cycle.
;;; :action-fn generates the command when the goal is active.

(setf *all-goals* nil)

(defgoal :maintain-torpedo-stock
  :priority 1
  :supported-by :win-game
  :satisfied (>= (fleet-torpedo-ratio slate) (theta 'theta-torpedo-ratio-target))
  :precondition t
  :action nil
  :needed-resources nil
  :doc "Fleet torpedo reserves above operational threshold.")

(defgoal :resupply-torpedoes
  :priority 2
  :supported-by :maintain-torpedo-stock
  :satisfied (null (find-resupplyable-ship-for-goal slate))
  :precondition (and (not (goal-satisfied-p :maintain-torpedo-stock))
                     (find-resupplyable-ship-for-goal slate))
  :action (issue-resupply-action slate)
  :needed-resources nil
  :doc "Reload torpedoes at own base.")

(defgoal :fabricate-materiel
  :priority 3
  :supported-by :maintain-torpedo-stock
  :satisfied (goal-satisfied-p :maintain-torpedo-stock)
  :precondition (and (not (goal-satisfied-p :maintain-torpedo-stock))
                     (find-fabricatable-ship-for-goal slate))
  :action (issue-fabricate-action slate)
  :needed-resources *torpedo-recipe-resources*
  :doc "Convert resources to torpedoes at shipyard.")

(defgoal :acquire-resource
  :priority 4
  :supported-by :fabricate-materiel
  :satisfied (null (torpedo-ingredients-needed slate))
  :precondition (and (not (goal-satisfied-p :acquire-resource))
                     (not (goal-satisfied-p :maintain-torpedo-stock))
                     (or (find-extractable-for-goal slate)
                         (find-buyable-for-goal slate)))
  :action (issue-acquire-action slate)
  :needed-resources (mapcar #'car (torpedo-ingredients-needed slate))
  :doc "Extract or buy needed torpedo ingredients.")

(defgoal :survey-for-yield
  :priority 5
  :supported-by :acquire-resource
  :satisfied (null (find-surveyable-for-goal slate))
  :precondition (and (not (goal-satisfied-p :acquire-resource))
                     (not (goal-satisfied-p :maintain-torpedo-stock))
                     (find-surveyable-for-goal slate))
  :action (issue-survey-action slate)
  :needed-resources nil
  :doc "Survey systems for better extraction yield.")

(defgoal :salvage-opportunity
  :priority 6
  :supported-by :win-game
  :satisfied (null (find-salvage-opportunity slate))
  :precondition (find-salvage-opportunity slate)
  :action (issue-salvage-action slate)
  :needed-resources nil
  :doc "Salvage debris at current hex.")

(defgoal :sell-excess-cargo
  :priority 8
  :supported-by :win-game
  :satisfied (null (find-excess-cargo-ship slate))
  :precondition (find-excess-cargo-ship slate)
  :action (issue-sell-action slate goals)
  :needed-resources nil
  :doc "Sell cargo serving no active goal.")

;;; ----------------------------------------------------------------------------
;;; Goal Engine
;;; ----------------------------------------------------------------------------

(defun goal-satisfied-p (objective)
  "Check if the goal with OBJECTIVE keyword is currently satisfied.
   Requires *current-slate* to be bound during goal evaluation."
  (let ((gdef (find objective *all-goals*
                    :key (lambda (g) (getf g :objective)))))
    (if gdef
        (funcall (getf gdef :satisfied-fn) *current-slate*)
        t)))

(defun evaluate-all-goals (slate)
  "Evaluate all goals from *all-goals* registry against current state.
   Returns list of goal plists sorted by priority.
   This is the CALCULATE step of the MPC loop."
  (let ((*current-slate* slate)
        (goals nil))
    (dolist (gdef *all-goals*)
      (push (list :objective (getf gdef :objective)
                  :priority (getf gdef :priority)
                  :satisfied (funcall (getf gdef :satisfied-fn) slate)
                  :relevant (funcall (getf gdef :precondition-fn) slate)
                  :needed-resources (funcall (getf gdef :needed-resources-fn) slate))
            goals))
    (setf goals (sort goals (lambda (a b)
                              (< (getf a :priority) (getf b :priority)))))
    (dolist (g goals)
      (format t "[LISP] Goal ~A: sat=~A rel=~A pri=~A~%"
              (getf g :objective) (getf g :satisfied) (getf g :relevant)
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

(defun torpedo-ingredients-needed (slate)
  "Return alist of (resource-type . qty-short) for torpedo recipe.
   Only includes resources where fleet has less than recipe requires."
  (let ((needs nil))
    (dolist (ingredient *torpedo-recipe*)
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
  (let ((needed (torpedo-ingredients-needed slate)))
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
  (let ((needed (torpedo-ingredients-needed slate)))
    (when (and needed (> (slate-credits slate) (theta 'theta-buy-credits-min)))
      (dolist (ship (slate-own-ships slate))
        (let ((sys (ship-at-system ship)))
          (when (and (not (string= sys ""))
                     (can-trade-at-p slate sys)
                     (> (ship-cargo-free-space ship) 0))
            (return-from find-buyable-for-goal
              (list ship (caar needed)))))))))

(defun find-fabricatable-ship-for-goal (slate)
  "Find ship at shipyard with all torpedo ingredients."
  (let ((player (slate-get slate :aa-player)))
    (dolist (ship (slate-own-ships slate))
      (let ((sys (ship-at-system ship)))
        (when (and (not (string= sys ""))
                   (can-fabricate-at-p slate sys player)
                   (>= (ship-cargo-ferrous ship) (theta 'theta-torpedo-recipe-ferrous))
                   (>= (ship-cargo-radioactive ship) (theta 'theta-torpedo-recipe-radioactive))
                   (>= (ship-cargo-volatile ship) (theta 'theta-torpedo-recipe-volatile)))
          (return-from find-fabricatable-ship-for-goal ship)))))
  nil)

(defun find-excess-cargo-ship (slate)
  "Find ship at trade hub with cargo that serves no active goal.
   Evaluates cargo against active goals to determine excess."
  (let ((goals (list
                 (list :objective :fabricate-materiel
                       :satisfied (>= (fleet-torpedo-ratio slate)
                                      (theta 'theta-torpedo-ratio-target))
                       :relevant t
                       :needed-resources *torpedo-recipe-resources*)
                 (list :objective :acquire-resource
                       :satisfied (null (torpedo-ingredients-needed slate))
                       :relevant t
                       :needed-resources (mapcar #'car (torpedo-ingredients-needed slate))))))
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
  "Find ship at own base needing torpedoes."
  (let ((own-bases (slate-own-bases slate)))
    (dolist (ship (slate-own-ships slate))
      (let ((hex (ship-hex ship))
            (torpedoes (ship-torpedo ship))
            (max-torpedoes (ship-torpedoes-max ship)))
        (when (and (member hex own-bases :test #'string=)
                   (> max-torpedoes 0)
                   (< torpedoes max-torpedoes)
                   (< (/ (float torpedoes) max-torpedoes) (theta 'theta-resupply-threshold)))
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

;;; ----------------------------------------------------------------------------
;;; Goal Graph Validation (load-time check)
;;; ----------------------------------------------------------------------------

(defun validate-goal-graph ()
  "Check all goals reach :win-game via :supported-by, no cycles.
   Called at load time."
  (let ((errors nil))
    (dolist (gdef *all-goals*)
      (let* ((obj (getf gdef :objective))
             (chain nil)
             (current obj))
        ;; Walk :supported-by chain to :win-game
        (loop
          (when (eq current :win-game) (return))
          (when (member current chain)
            (push (format nil "Goal ~A has cycle in :supported-by chain" obj) errors)
            (return))
          (push current chain)
          (let ((parent (find current *all-goals*
                              :key (lambda (g) (getf g :objective)))))
            (if parent
                (setf current (getf parent :supported-by))
                (progn
                  (push (format nil "Goal ~A :supported-by ~A not found" obj current) errors)
                  (return)))))))
    (if errors
        (dolist (e errors)
          (format t "[LISP] GOAL GRAPH ERROR: ~A~%" e))
        (format t "[LISP] Goal graph validated: ~A goals, all reach :win-game~%"
                (length *all-goals*)))))

(validate-goal-graph)
