;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-entities.lisp - Gen4 Entity Ontology Layer
;;;;
;;;; Declares all entity types via defentity macro.
;;;; Generates accessor functions identical to the hand-written ones
;;;; they replace in aa-util.lisp.
;;;;
;;;; Depends on: aa-macros.lisp (defentity macro)

;;; ============================================================================
;;; Ship Entity (~42 fields, replaces ~48 hand-written accessors)
;;; ============================================================================
;;; Field ordering matters: basic fields first, derived fields last
;;; (derived forms reference accessors defined by earlier fields).

(defentity ship
  "A ship (own, enemy, or draft). Plist from C++ gather step."

  ;; --- Identity ---
  (:name :type string)
  (:code :type string :transform string-downcase)
  (:hex :type string :transform string-downcase)

  ;; --- Core Stats ---
  (:pd :type integer :default 0)
  (:phasic :alias :b :type integer :default 0)
  (:shield :alias :s :type integer :default 0)
  (:launcher :alias :t :type integer :default 0)
  (:torpedo :alias :m :type integer :default 0)
  (:hangar :type integer :default 0)
  (:tech :type integer :default 0)

  ;; --- Max Stats (for damage/repair) ---
  (:base-pd :type integer :default-form (ship-pd ship))
  (:pd-max :type integer :default-form (ship-pd ship))
  (:phasic-max :type integer :default-form (ship-phasic ship))
  (:shield-max :type integer :default-form (ship-shield ship))
  (:launcher-max :type integer :default-form (ship-launcher ship))

  ;; --- Warp / Rack ---
  (:warpship-p :alias :warpship)
  (:racked :type list)
  (:racked-in :type string :transform string-downcase)
  (:racked-p :derived (let ((racked (getf ship :racked-in)))
                        (and racked (not (string= racked "")))))

  ;; --- Location ---
  (:at-system :type string :transform string-downcase)
  (:suggested-dest :type string :transform string-downcase)

  ;; --- Cargo ---
  (:cargo-ferrous :type integer :default 0)
  (:cargo-rare-earth :type integer :default 0)
  (:cargo-radioactive :type integer :default 0)
  (:cargo-crystalline :type integer :default 0)
  (:cargo-volatile :type integer :default 0)
  (:cargo-water :type integer :default 0)
  (:cargo-organic :type integer :default 0)
  (:cargo-exotic :type integer :default 0)
  (:cargo-torpedoes :type integer :default 0)
  (:cargo-capacity :type integer :default 10)
  (:torpedoes-max :type integer :default 0)

  ;; --- Derived Cargo ---
  (:cargo-total :derived (+ (ship-cargo-ferrous ship)
                            (ship-cargo-rare-earth ship)
                            (ship-cargo-radioactive ship)
                            (ship-cargo-crystalline ship)
                            (ship-cargo-volatile ship)
                            (ship-cargo-water ship)
                            (ship-cargo-organic ship)
                            (ship-cargo-exotic ship)
                            (ship-cargo-torpedoes ship)))
  (:cargo-free-space :derived (- (ship-cargo-capacity ship)
                                 (ship-cargo-total ship)))

  ;; --- Combat State ---
  (:needs-order-p :alias :needs-order)
  (:pending-damage :type integer :default 0)
  (:escape-pending-p :alias :escape-pending)

  ;; --- Revealed Enemy Orders ---
  (:last-tactic :type character)
  (:last-drive :type integer :default 0)
  (:last-phasic :type integer :default 0)
  (:last-shield :type integer :default 0)
  (:last-launcher :type integer :default 0))

;;; ============================================================================
;;; Combat Entity (active combat instance at a hex)
;;; ============================================================================

(defentity combat
  "An active combat instance."
  (:hex :type string)
  (:stage :type integer :default 0)
  (:round :type integer :default 1)
  (:ai-committed-p :alias :ai-committed)
  (:stalemate-count :alias :stalemate :type integer :default 0)
  (:ai-attacker-p :alias :ai-attacker))

;;; ============================================================================
;;; Resource Entity
;;; ============================================================================

(defentity resource
  "A resource at a star system."
  (:system :type string)
  (:type :type string)
  (:abundance :type string)
  (:yield :type integer :default 1))

;;; ============================================================================
;;; Facility Entity
;;; ============================================================================

(defentity facility
  "A facility at a star system."
  (:type :type string)
  (:controller :type integer)
  (:system :type string))

;;; ============================================================================
;;; Market Price Entity
;;; ============================================================================

(defentity market-price
  "A market price entry."
  (:resource :alias :type :type string)
  (:current :alias :price :type integer :default 0)
  (:base :alias :base-price :type integer :default 0))

;;; ============================================================================
;;; Salvageable Entity
;;; ============================================================================

(defentity salvageable
  "A salvageable object at a hex."
  (:hex :type string)
  (:object-type :type string)
  (:state :type string)
  (:value :type integer :default 0))

;;; ============================================================================
;;; Codex Entry Entity
;;; ============================================================================

(defentity codex-entry
  "A codex entry for a star system."
  (:system :type string)
  (:level :type string))
