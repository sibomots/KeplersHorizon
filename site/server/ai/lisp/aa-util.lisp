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

(defun slate-active-combats (slate)
  (slate-get slate :active-combats nil))

;;; ----------------------------------------------------------------------------
;;; Ship Accessors
;;; ----------------------------------------------------------------------------

(defun ship-code (ship)
  "Return ship code in lowercase for command generation."
  (let ((code (getf ship :code)))
    (if code (string-downcase code) "")))

(defun ship-name (ship)
  "Return ship name (preserves case for display)."
  (getf ship :name))

(defun ship-hex (ship)
  "Return hex ID in lowercase for command generation."
  (let ((hex (getf ship :hex)))
    (if hex (string-downcase hex) "")))

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
  "Get C++ computed suggested destination for this ship (lowercase)."
  (let ((dest (getf ship :suggested-dest)))
    (if dest (string-downcase dest) "")))

;; Combat state accessors
(defun ship-needs-order-p (ship)
  "Check if ship needs a combat order."
  (getf ship :needs-order))

(defun ship-pending-damage (ship)
  "Get pending damage to assign (0 if none)."
  (or (getf ship :pending-damage) 0))

(defun ship-escape-pending-p (ship)
  "Check if ship has escaped and needs retreat command."
  (getf ship :escape-pending))

;; Revealed enemy order accessors (public after both players commit)
(defun ship-last-tactic (ship)
  "Get enemy's tactic from prior round (A, D, E, or NIL if unknown)."
  (getf ship :last-tactic))

(defun ship-last-drive (ship)
  "Get enemy's drive power from prior round."
  (or (getf ship :last-drive) 0))

(defun ship-last-beam (ship)
  "Get enemy's beam power from prior round."
  (or (getf ship :last-beam) 0))

;;; ----------------------------------------------------------------------------
;;; Combat Hex Accessors
;;; ----------------------------------------------------------------------------

(defun combat-hex (ch)
  (getf ch :hex))

(defun combat-stage (ch)
  (or (getf ch :stage) 0))

(defun combat-round (ch)
  (or (getf ch :round) 1))

(defun combat-ai-committed-p (ch)
  (getf ch :ai-committed))

(defun combat-stalemate-count (ch)
  "Get consecutive no-damage round count."
  (or (getf ch :stalemate) 0))

(defun combat-ai-attacker-p (ch)
  "Check if AI is the attacker (moved into hex, has initiative)."
  (getf ch :ai-attacker))

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
  (make-cmd "next"))

(defun cmd-done ()
  (make-cmd "done"))
