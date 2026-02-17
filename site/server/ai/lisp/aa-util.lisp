;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-util.lisp - Utility predicates and helpers (Gen4)
;;;;
;;;; Entity accessors moved to aa-entities.lisp (defentity macro).
;;;; This file retains: slate accessors, compound lookups, list filters,
;;;; predicates, hex helpers, and command helpers.

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

(defun slate-home-side (slate)
  (slate-get slate :home-side ""))

(defun slate-contested-hexes (slate)
  (slate-get slate :contested-hexes nil))

(defun slate-in-combat (slate)
  (slate-get slate :in-combat nil))

(defun slate-active-combats (slate)
  (slate-get slate :active-combats nil))

(defun slate-vp (slate)
  (slate-get slate :vp 0))

(defun slate-enemy-vp (slate)
  (slate-get slate :enemy-vp 0))

;;; ----------------------------------------------------------------------------
;;; Cross-Turn Metric Accessors
;;; ----------------------------------------------------------------------------

(defun slate-metrics (slate)
  "Get alist of persisted metrics: ((name . value) ...)"
  (slate-get slate :metrics nil))

(defun slate-metric (slate name &optional (default 0.0))
  "Get persisted metric by name. Returns DEFAULT if not found."
  (let ((entry (assoc name (slate-metrics slate) :test #'string-equal)))
    (if entry (cdr entry) default)))

(defun slate-distances (slate)
  "Get alist of precomputed BFS distances: (((from . to) . cost) ...)"
  (slate-get slate :distances nil))

(defun slate-distance (slate from-hex to-hex)
  "Get BFS hop count between two hexes. Returns 999 if not precomputed.
   Searches both (from . to) and (to . from) since base-to-base pairs
   are stored in one direction only."
  (let ((dists (slate-distances slate)))
    (dolist (entry dists 999)
      (let ((key (car entry)))
        (when (consp key)
          (when (or (and (string-equal (car key) from-hex)
                        (string-equal (cdr key) to-hex))
                   (and (string-equal (car key) to-hex)
                        (string-equal (cdr key) from-hex)))
            (return-from slate-distance (cdr entry))))))))

(defun slate-enemy-distance (slate enemy-code base-hex)
  "Get BFS cost for enemy ship to reach our base. Returns 999 if not found.
   Looks up enemy ship hex, then queries general distance matrix."
  (let ((enemy (find enemy-code (slate-enemy-ships slate)
                     :key (lambda (s) (getf s :code))
                     :test #'string-equal)))
    (if enemy
        (let ((enemy-hex (ship-hex enemy)))
          (if (and enemy-hex (not (string= enemy-hex "")))
              (slate-distance slate enemy-hex base-hex)
              999))
        999)))

(defun slate-warpline-hexes (slate)
  "Get list of hexes that lie on warplines."
  (slate-get slate :warpline-hexes nil))

(defun warpline-hex-p (slate hex)
  "T if hex lies on a warpline."
  (member hex (slate-warpline-hexes slate) :test #'string-equal))

(defun slate-adjacency (slate)
  "Get alist of (hex-id . (neighbor1 neighbor2 ...))."
  (slate-get slate :adjacency nil))

(defun hex-neighbors (slate hex)
  "Get list of hexes adjacent to HEX (geometric + warpline).
   Returns NIL if hex not in adjacency data."
  (let ((entry (assoc hex (slate-adjacency slate) :test #'string-equal)))
    (if entry (cdr entry) nil)))

;;; ----------------------------------------------------------------------------
;;; Economic Layer: Codex Accessors
;;; ----------------------------------------------------------------------------

(defun slate-codex (slate)
  "Get list of codex entries."
  (slate-get slate :codex nil))

(defun get-knowledge-level (slate system-name)
  "Get knowledge level for a system. Returns 'Unknown' if not in codex."
  (let ((entry (find system-name (slate-codex slate)
                     :key (lambda (e) (getf e :system))
                     :test #'string-equal)))
    (if entry
        (getf entry :level)
        "Unknown")))

(defun knowledge-level-value (level)
  "Convert knowledge level to numeric value for comparison."
  (cond
    ((string-equal level "Unknown") 0)
    ((string-equal level "Charted") 1)
    ((string-equal level "Surveyed") 2)
    ((string-equal level "Intimate") 3)
    (t 0)))

;;; ----------------------------------------------------------------------------
;;; Economic Layer: Resource Accessors
;;; ----------------------------------------------------------------------------

(defun slate-resources (slate)
  "Get list of resources at ship locations."
  (slate-get slate :resources nil))

(defun resources-at-system (slate system-name)
  "Get resources available at a specific system."
  (remove-if-not (lambda (r)
                   (string-equal (getf r :system) system-name))
                 (slate-resources slate)))


;;; ----------------------------------------------------------------------------
;;; Economic Layer: Facility Accessors
;;; ----------------------------------------------------------------------------

(defun slate-facilities (slate)
  "Get list of facilities."
  (slate-get slate :facilities nil))

(defun facilities-at-system (slate system-name)
  "Get facilities at a specific system."
  (remove-if-not (lambda (f)
                   (string-equal (getf f :system) system-name))
                 (slate-facilities slate)))


(defun can-fabricate-at-p (slate system-name player)
  "Check if player can fabricate at this system (SHIPYARD or REFINERY)."
  (some (lambda (fac)
          (and (string-equal (getf fac :system) system-name)
               (or (string-equal (getf fac :type) "SHIPYARD")
                   (string-equal (getf fac :type) "REFINERY"))
               (eql (getf fac :controller) player)))
        (slate-facilities slate)))

(defun can-trade-at-p (slate system-name)
  "Check if there's a TRADE_HUB at this system."
  (some (lambda (fac)
          (and (string-equal (getf fac :system) system-name)
               (string-equal (getf fac :type) "TRADE_HUB")))
        (slate-facilities slate)))

;;; ----------------------------------------------------------------------------
;;; Economic Layer: Market Price Accessors
;;; ----------------------------------------------------------------------------

(defun slate-market-prices (slate)
  "Get list of market prices."
  (slate-get slate :market-prices nil))


(defun get-market-price (slate resource-type)
  "Find market price entry for a resource type."
  (find resource-type (slate-market-prices slate)
        :key #'market-price-resource
        :test #'string-equal))

;;; ----------------------------------------------------------------------------
;;; Economic Layer: Salvageable Accessors
;;; ----------------------------------------------------------------------------

(defun slate-salvageables (slate)
  "Get list of salvageable objects."
  (slate-get slate :salvageables nil))


(defun salvageables-at-hex (slate hex)
  "Get salvageable objects at a specific hex."
  (remove-if-not (lambda (sv)
                   (string-equal (getf sv :hex) hex))
                 (slate-salvageables slate)))


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
;;; Hex ID Helpers
;;; ----------------------------------------------------------------------------

(defun ensure-hex-prefix (hex-id)
  "Ensure hex ID has 'h' prefix."
  (if (or (null hex-id) (string= hex-id ""))
      hex-id
      (if (char= (char hex-id 0) #\h)
          hex-id
          (concatenate 'string "h" hex-id))))

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

(defun make-metric (name value)
  "Create a __metric write-back pseudo-command.
   Intercepted by C++ unmarshal, not injected into game engine."
  (list :cmd "__metric" :args (format nil "~A ~A" name (float value))))
