;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This file is part of Kepler's Horizon ;;
;;                                       ;;
;; Licensed under BSD 3-Clause License   ;;
;;                                       ;;
;; Copyright (c) 2025, sibomots          ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; aa-theta.lisp - Gen4 Named Parameter Vector
;;;;
;;;; Every hardcoded numeric literal from the AI codebase is named here.
;;;; Same values as Gen3 — no behavior change.
;;;; Access: (theta 'theta-xxx)
;;;; Errors on unknown key (catches typos at load time).
;;;;
;;;; Depends on: aa-macros.lisp (loaded before this file)

;;; ============================================================================
;;; Parameter Vector
;;; ============================================================================

(defparameter *theta*
  '(
    ;; -----------------------------------------------------------------------
    ;; Combat Power Weights (aa-strategy.lisp ship-combat-power)
    ;; -----------------------------------------------------------------------
    (theta-pd-weight . 1.0)
    (theta-beam-weight . 1.5)
    (theta-screen-weight . 1.2)
    (theta-tech-weight . 2.5)
    (theta-tube-weight . 0.5)
    (theta-missile-weight . 0.3)

    ;; -----------------------------------------------------------------------
    ;; Hex Valuation Bonuses (aa-strategy.lisp compute-hex-value)
    ;; -----------------------------------------------------------------------
    (theta-hex-enemy-base . 100)
    (theta-hex-own-base . 80)
    (theta-hex-shipyard . 30)
    (theta-hex-refinery . 20)
    (theta-hex-repair-dock . 15)
    (theta-hex-trade-hub . 10)
    (theta-hex-proximity-bonus . 10)
    (theta-hex-proximity-range . 4)
    (theta-hex-warpline-bonus . 15)
    (theta-hex-resource-high-value . 20)
    (theta-hex-resource-high-threshold . 8)
    (theta-hex-resource-med-value . 10)
    (theta-hex-resource-med-threshold . 4)
    (theta-hex-resource-low-value . 5)
    (theta-hex-resource-low-threshold . 2)
    (theta-hex-chokepoint-bonus . 25)
    (theta-hex-denial-bonus . 10)
    (theta-hex-vp-occupied-bonus . 40)

    ;; -----------------------------------------------------------------------
    ;; Theater Priorities (aa-strategy.lisp compute-theater-priorities)
    ;; -----------------------------------------------------------------------
    (theta-theater-defense-bonus . 50)
    (theta-theater-vp-bonus . 40)
    (theta-theater-reinforce-bonus . 30)

    ;; -----------------------------------------------------------------------
    ;; Force Projection (aa-strategy.lisp compute-force-projection)
    ;; -----------------------------------------------------------------------
    (theta-force-threat-distance . 6)

    ;; -----------------------------------------------------------------------
    ;; Temporal / Game Phase (aa-strategy.lisp compute-temporal-state)
    ;; -----------------------------------------------------------------------
    (theta-tech-period . 4)
    (theta-wait-for-tech-credits . 15)
    (theta-phase-early-end . 3)
    (theta-phase-mid-end . 10)
    (theta-theater-winning-ratio . 1.3)
    (theta-theater-losing-ratio . 0.7)

    ;; -----------------------------------------------------------------------
    ;; Enemy Intent (aa-strategy.lisp compute-enemy-intent)
    ;; -----------------------------------------------------------------------
    (theta-enemy-near-threshold . 6)
    (theta-enemy-converge-aggressive . 3)
    (theta-enemy-converge-minimum . 2)
    (theta-enemy-wg-bp-cost . 5)
    (theta-enemy-stable-rounds . 3.0)

    ;; -----------------------------------------------------------------------
    ;; Posture Thresholds (aa-strategy.lisp compute-posture)
    ;; -----------------------------------------------------------------------
    (theta-posture-vp-behind . 2)
    (theta-posture-tech-aggressive . 1.0)
    (theta-posture-tech-defensive . -1.0)

    ;; -----------------------------------------------------------------------
    ;; Metrics / Attrition (aa-strategy.lisp compute-metrics-to-persist)
    ;; -----------------------------------------------------------------------
    (theta-attrition-alpha . 0.4)
    (theta-attrition-beta . 0.6)

    ;; -----------------------------------------------------------------------
    ;; Combat Triage (aa-combat.lisp assess-theater)
    ;; -----------------------------------------------------------------------
    (theta-triage-vp-fight-ratio . 0.5)
    (theta-triage-advantage-ratio . 1.2)
    (theta-triage-acceptable-fight-ratio . 0.4)
    (theta-triage-matched-ratio . 0.7)
    (theta-triage-retreat-ratio . 0.7)

    ;; -----------------------------------------------------------------------
    ;; CRT Allocation (aa-combat.lisp crt-attack-alloc)
    ;; -----------------------------------------------------------------------
    (theta-crt-standard-diff . 2)
    (theta-crt-pursue-diff . 4)
    (theta-crt-counter-dodge-diff . 3)
    (theta-crt-max-safe-diff . 4)

    ;; -----------------------------------------------------------------------
    ;; Dodge Allocation (aa-combat.lisp crt-dodge-alloc)
    ;; -----------------------------------------------------------------------
    (theta-dodge-drive-fraction . 0.4)
    (theta-dodge-screen-fraction . 0.5)

    ;; -----------------------------------------------------------------------
    ;; Auto-Retreat Thresholds (aa-combat.lisp should-auto-retreat-p)
    ;; -----------------------------------------------------------------------
    (theta-retreat-threshold-low . 0.60)
    (theta-retreat-threshold-normal . 0.40)
    (theta-retreat-threshold-high . 0.25)

    ;; -----------------------------------------------------------------------
    ;; Combat Analysis (aa-combat.lisp analyze-combat-situation)
    ;; -----------------------------------------------------------------------
    (theta-combat-outmatched-ratio . 0.5)
    (theta-combat-advantage-ratio . 1.5)
    (theta-enemy-pd-default . 3)
    (theta-alpha-strike-screen-threshold . 3)

    ;; -----------------------------------------------------------------------
    ;; Screen Penetration (aa-combat.lisp minimum-ships-to-penetrate)
    ;; -----------------------------------------------------------------------
    (theta-brawler-damage-estimate . 5)
    (theta-heavy-screen-threshold . 4)

    ;; -----------------------------------------------------------------------
    ;; Build Phase (aa-build.lisp)
    ;; -----------------------------------------------------------------------
    (theta-minimum-reserve . 10)
    (theta-warpship-cost . 17)
    (theta-defender-cost . 13)
    (theta-wait-tech-turns . 2)
    (theta-wait-tech-ships-min . 2)
    (theta-wait-tech-credits-min . 20)
    (theta-max-defenders . 2)
    (theta-defender-base-threshold . 2)

    ;; -----------------------------------------------------------------------
    ;; Ship Design (aa-build.lisp choose-ship-design)
    ;; -----------------------------------------------------------------------
    (theta-design-tech-threshold . 1)
    (theta-missile-boats-cap . 1)
    (theta-brawler-tech-cap . 3)
    (theta-interceptor-cap . 2)
    (theta-fleet-variety-size . 3)

    ;; -----------------------------------------------------------------------
    ;; Ship Classification (aa-strategy.lisp classify-ship-role)
    ;; -----------------------------------------------------------------------
    (theta-classify-fortress-pd . 7)
    (theta-classify-brawler-pd . 5)
    (theta-classify-brawler-beam . 3)

    ;; -----------------------------------------------------------------------
    ;; Goals / Missiles (aa-goals.lisp)
    ;; -----------------------------------------------------------------------
    (theta-missile-ratio-target . 0.5)
    (theta-resupply-threshold . 0.5)
    (theta-resupply-max . 6)
    (theta-missile-recipe-ferrous . 2)
    (theta-missile-recipe-radioactive . 1)
    (theta-missile-recipe-volatile . 1)

    ;; -----------------------------------------------------------------------
    ;; Economic (aa-economic.lisp)
    ;; -----------------------------------------------------------------------
    (theta-buy-quantity . 2)
    (theta-buy-credits-min . 50)
    (theta-sell-price-floor . 0.8)

    ;; -----------------------------------------------------------------------
    ;; Movement (aa-movement.lisp)
    ;; -----------------------------------------------------------------------
    (theta-movement-threat-range . 3)
    ))

;;; ============================================================================
;;; Theta Accessor
;;; ============================================================================

(defun theta (key)
  "Look up named parameter. Errors on unknown key (catches typos at load time)."
  (let ((entry (assoc key *theta*)))
    (if entry
        (cdr entry)
        (error "Unknown theta parameter: ~A" key))))

;;; ============================================================================
;;; Ship Design Templates
;;; ============================================================================
;;; Replaces *brawler-spec*, *interceptor-spec*, etc. in aa-build.lisp

(defparameter *ship-templates*
  '((:brawler      . "PD=6 B=4 S=3 T=0 M=0")
    (:interceptor  . "PD=5 B=2 S=2 T=0 M=0")
    (:missile-boat . "PD=4 B=0 S=1 T=2 M=6")
    (:fortress     . "PD=8 B=6 S=5 T=0 M=0")
    (:defender     . "PD=6 B=4 S=3 T=0 M=0")))

(defun get-ship-template (role)
  "Get ship design spec string for a role keyword."
  (let ((entry (assoc role *ship-templates*)))
    (if entry
        (cdr entry)
        (error "Unknown ship template role: ~A" role))))
