;;;; aa-util.lisp - Utility predicates and helpers

;;; ----------------------------------------------------------------------------
;;; Slate Accessors
;;; ----------------------------------------------------------------------------

(defun slate-get (slate key &optional default)
  "Get value from slate alist."
  (let ((pair (assoc key slate)))
    (if pair (cdr pair) default)))

(defun slate-credits (slate)
  (slate-get slate :credits 0))

(defun slate-phase (slate)
  (slate-get slate :phase 0))

(defun slate-round (slate)
  (slate-get slate :round 1))

(defun slate-tech-level (slate)
  (slate-get slate :tech-level 0))

(defun slate-own-ships (slate)
  (slate-get slate :own-ships nil))

(defun slate-enemy-ships (slate)
  (slate-get slate :enemy-ships nil))

(defun slate-drafts (slate)
  (slate-get slate :drafts nil))

(defun slate-own-bases (slate)
  (slate-get slate :own-bases nil))

(defun slate-enemy-bases (slate)
  (slate-get slate :enemy-bases nil))

(defun slate-contested-hexes (slate)
  (slate-get slate :contested-hexes nil))

(defun slate-in-combat (slate)
  (slate-get slate :in-combat nil))

;;; ----------------------------------------------------------------------------
;;; Ship Accessors
;;; ----------------------------------------------------------------------------

(defun ship-code (ship)
  (getf ship :code))

(defun ship-name (ship)
  (getf ship :name))

(defun ship-hex (ship)
  (getf ship :hex))

(defun ship-pd (ship)
  (or (getf ship :pd) 0))

(defun ship-beam (ship)
  (or (getf ship :b) 0))

(defun ship-screen (ship)
  (or (getf ship :s) 0))

(defun ship-tube (ship)
  (or (getf ship :t) 0))

(defun ship-missile (ship)
  (or (getf ship :m) 0))

(defun ship-sr (ship)
  (or (getf ship :sr) 0))

(defun ship-tech (ship)
  (or (getf ship :tech) 0))

(defun ship-warpship-p (ship)
  (getf ship :warpship))

(defun ship-suggested-dest (ship)
  "Get C++ computed suggested destination for this ship."
  (getf ship :suggested-dest))

;;; ----------------------------------------------------------------------------
;;; Predicates
;;; ----------------------------------------------------------------------------

(defun can-afford-p (slate cost)
  "Check if we can afford COST credits."
  (>= (slate-credits slate) cost))

(defun has-drafts-p (slate)
  "Check if there are pending ship drafts."
  (not (null (slate-drafts slate))))

(defun in-combat-p (slate)
  "Check if in combat."
  (slate-in-combat slate))

(defun has-ships-p (slate)
  "Check if we have any ships."
  (not (null (slate-own-ships slate))))

(defun ships-at-hex (ships hex)
  "Filter ships at a specific hex."
  (remove-if-not (lambda (s) (string= (ship-hex s) hex)) ships))

(defun first-base (slate)
  "Get first owned base hex."
  (first (slate-own-bases slate)))

;;; ----------------------------------------------------------------------------
;;; Command Helpers
;;; ----------------------------------------------------------------------------

(defun make-cmd (cmd &optional args)
  "Create a command spec."
  (list :cmd cmd :args (or args "")))

(defun cmd-next ()
  (make-cmd "NEXT"))

(defun cmd-done ()
  (make-cmd "DONE"))
