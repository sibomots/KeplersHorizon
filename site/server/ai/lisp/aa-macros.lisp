;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-macros.lisp - Gen4 DSL Infrastructure
;;;;
;;;; Three macros define the AI agent's declarative knowledge:
;;;;   defentity          — entity ontology (accessor generation)
;;;;   define-strategy-rule — named decision rules
;;;;   defgoal            — goal graph declarations
;;;;
;;;; Loaded first. No dependencies on other AI modules.

;;; ============================================================================
;;; Global Registries
;;; ============================================================================

(defvar *strategy-rules* nil
  "Registry of all strategy rules. Populated by define-strategy-rule.")

(defvar *all-goals* nil
  "Registry of all goal declarations. Populated by defgoal.")

(defvar *fired-rules-this-cycle* nil
  "List of rule names fired during current CALCULATE cycle.")

;;; ============================================================================
;;; defentity — Entity Ontology Macro
;;; ============================================================================
;;;
;;; Each field spec is a list:
;;;   (field-name &key alias default default-form transform derived type)
;;;
;;;   :alias KEY       — read from KEY instead of field-name
;;;   :default VALUE   — fallback when getf returns nil
;;;   :default-form FORM — computed fallback (may reference entity parameter)
;;;   :transform FN    — apply FN with nil-guard (returns "" on nil)
;;;   :derived FORM    — whole accessor body is FORM (no getf)
;;;   :type TYPE       — documentation only, not used in expansion
;;;
;;; Expands to:
;;;   (defparameter *entity-fields* '(field-names...))
;;;   (defun entity-field (entity) ...)   ; one per field

(defmacro defentity (name docstring &body fields)
  "Define an entity type with auto-generated accessor functions."
  (declare (ignore docstring))
  (let* ((fields-var (intern (concatenate 'string
                               "*" (symbol-name name) "-FIELDS*")))
         (field-keywords nil)
         (defuns nil))
    (dolist (field fields)
      (let* ((field-name (first field))
             (field-props (rest field))
             (alias (getf field-props :alias))
             (plist-key (or alias field-name))
             (default-val (getf field-props :default))
             (default-form (getf field-props :default-form))
             (transform (getf field-props :transform))
             (derived (getf field-props :derived))
             (fn-name (intern (concatenate 'string
                                (symbol-name name) "-"
                                (symbol-name field-name)))))
        (push field-name field-keywords)
        (push
         (cond
           ;; Derived: body is the derived form directly
           (derived
            `(defun ,fn-name (,name) ,derived))
           ;; Transform with nil guard (returns "" on nil)
           (transform
            `(defun ,fn-name (,name)
               (let ((v (getf ,name ,plist-key)))
                 (if v (,transform v) ""))))
           ;; Computed default-form
           (default-form
            `(defun ,fn-name (,name)
               (or (getf ,name ,plist-key) ,default-form)))
           ;; Static default
           (default-val
            `(defun ,fn-name (,name)
               (or (getf ,name ,plist-key) ,default-val)))
           ;; Simple access (no default, no transform)
           (t
            `(defun ,fn-name (,name) (getf ,name ,plist-key))))
         defuns)))
    `(progn
       (defparameter ,fields-var ',(nreverse field-keywords))
       ,@(nreverse defuns))))

;;; ============================================================================
;;; define-strategy-rule — Named Decision Rule Macro
;;; ============================================================================
;;;
;;; Rules are registered in *strategy-rules* and dispatched by phase.
;;; The rule engine evaluates :when lambdas and fires :action on match.
;;;
;;; :when  — lambda (slate strategy) returning generalized boolean
;;; :action — lambda (slate strategy) returning command list or value

(defmacro define-strategy-rule (name &key phase priority when action doc)
  "Register a named strategy rule in *strategy-rules*."
  `(push (list :name ',name
               :phase ,phase
               :priority ,priority
               :when (lambda (slate strategy)
                       (declare (ignorable slate strategy))
                       ,when)
               :action (lambda (slate strategy)
                         (declare (ignorable slate strategy))
                         ,action)
               :doc ,doc)
         *strategy-rules*))

;;; ============================================================================
;;; defgoal — Goal Graph Declaration Macro
;;; ============================================================================
;;;
;;; Goals form a dependency graph rooted at :win-game.
;;; Each goal has satisfaction and precondition predicates plus an action.
;;;
;;; :satisfied-fn    — lambda (slate) returning T when goal is met
;;; :precondition-fn — lambda (slate) returning T when goal is actionable
;;; :action-fn       — lambda (slate goals) returning command list or NIL

(defmacro defgoal (objective &key priority supported-by satisfied
                                  precondition action needed-resources doc)
  "Register a named goal in *all-goals*."
  `(push (list :objective ,objective
               :priority ,priority
               :supported-by ,supported-by
               :satisfied-fn (lambda (slate)
                               (declare (ignorable slate))
                               ,satisfied)
               :precondition-fn (lambda (slate)
                                  (declare (ignorable slate))
                                  ,precondition)
               :action-fn (lambda (slate goals)
                            (declare (ignorable slate goals))
                            ,action)
               :needed-resources-fn (lambda (slate)
                                      (declare (ignorable slate))
                                      ,needed-resources)
               :doc ,doc)
         *all-goals*))
