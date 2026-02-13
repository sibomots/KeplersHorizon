;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
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

(defun resource-type (res)
  (getf res :type))

(defun resource-abundance (res)
  (getf res :abundance))

(defun resource-yield (res)
  (or (getf res :yield) 1))

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

(defun facility-type (fac)
  (getf fac :type))

(defun facility-controller (fac)
  (getf fac :controller))

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

(defun market-price-resource (mp)
  (getf mp :type))

(defun market-price-current (mp)
  (or (getf mp :price) 0))

(defun market-price-base (mp)
  (or (getf mp :base-price) 0))

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

(defun salvageable-hex (sv)
  (getf sv :hex))

(defun salvageable-object-type (sv)
  (getf sv :object-type))

(defun salvageable-state (sv)
  (getf sv :state))

(defun salvageable-value (sv)
  (or (getf sv :value) 0))

(defun salvageables-at-hex (slate hex)
  "Get salvageable objects at a specific hex."
  (remove-if-not (lambda (sv)
                   (string-equal (getf sv :hex) hex))
                 (slate-salvageables slate)))

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
  "Remaining PD for power allocation (base - spent)."
  (or (getf ship :pd) 0))

(defun ship-base-pd (ship)
  "Physical PD for damage assignment (from ships table)."
  (or (getf ship :base-pd) (ship-pd ship)))

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

(defun ship-racked-in (ship)
  "Return the warpship code this systemship is racked in (empty if not racked)."
  (let ((racked (getf ship :racked-in)))
    (if racked (string-downcase racked) "")))

(defun ship-racked (ship)
  "Return list of systemship codes racked in this warpship."
  (getf ship :racked))

(defun ship-racked-p (ship)
  "Check if this systemship is racked in a warpship."
  (let ((racked (getf ship :racked-in)))
    (and racked (not (string= racked "")))))

;;; Economic layer: cargo
(defun ship-cargo-ferrous (ship)
  (or (getf ship :cargo-ferrous) 0))

(defun ship-cargo-rare-earth (ship)
  (or (getf ship :cargo-rare-earth) 0))

(defun ship-cargo-radioactive (ship)
  (or (getf ship :cargo-radioactive) 0))

(defun ship-cargo-crystalline (ship)
  (or (getf ship :cargo-crystalline) 0))

(defun ship-cargo-volatile (ship)
  (or (getf ship :cargo-volatile) 0))

(defun ship-cargo-water (ship)
  (or (getf ship :cargo-water) 0))

(defun ship-cargo-organic (ship)
  (or (getf ship :cargo-organic) 0))

(defun ship-cargo-exotic (ship)
  (or (getf ship :cargo-exotic) 0))

(defun ship-cargo-missiles (ship)
  (or (getf ship :cargo-missiles) 0))

(defun ship-cargo-capacity (ship)
  (or (getf ship :cargo-capacity) 10))

(defun ship-missiles-max (ship)
  (or (getf ship :missiles-max) 0))

(defun ship-at-system (ship)
  "Get the system name where ship is located."
  (let ((sys (getf ship :at-system)))
    (if sys (string-downcase sys) "")))

(defun ship-cargo-total (ship)
  "Get total cargo currently aboard ship."
  (+ (ship-cargo-ferrous ship)
     (ship-cargo-rare-earth ship)
     (ship-cargo-radioactive ship)
     (ship-cargo-crystalline ship)
     (ship-cargo-volatile ship)
     (ship-cargo-water ship)
     (ship-cargo-organic ship)
     (ship-cargo-exotic ship)
     (ship-cargo-missiles ship)))

(defun ship-cargo-free-space (ship)
  "Get available cargo space."
  (- (ship-cargo-capacity ship) (ship-cargo-total ship)))

;;; Max values for repair decisions
(defun ship-pd-max (ship)
  (or (getf ship :pd-max) (ship-pd ship)))

(defun ship-beam-max (ship)
  (or (getf ship :beam-max) (ship-beam ship)))

(defun ship-screen-max (ship)
  (or (getf ship :screen-max) (ship-screen ship)))

(defun ship-tube-max (ship)
  (or (getf ship :tube-max) (ship-tube ship)))

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

(defun ship-last-screen (ship)
  "Get enemy's screen power from prior round."
  (or (getf ship :last-screen) 0))

(defun ship-last-tube (ship)
  "Get enemy's tube power from prior round."
  (or (getf ship :last-tube) 0))

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
