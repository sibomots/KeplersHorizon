///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////

#ifndef __KH_STRING_DE_DE_H__
#define __KH_STRING_DE_DE_H__

#define LC_PARSER_ROOT_ERROR "Unbekannter Befehl. Gib 'help' ein für eine Liste der Befehle."
#define LC_CONFIGURE_PREFIX(x) "CONFIGURE: " x
#define LC_CONFIGURE_ACCESS_DENIED LC_CONFIGURE_PREFIX("Zugriff verweigert. Administratorrechte erforderlich.")
#define LC_CONFIGURE_BANNER LC_CONFIGURE_PREFIX("Aktuelle Einstellungen")
#define LC_CONFIGURE_SUBCMDS "Unterbefehle:"
#define LC_CONFIGURE_RELOAD_CONF "  configure reload conf  — kh.conf neu laden"
#define LC_CONFIGURE_AI_RELOAD "  configure reload ai    — AI DSL neu laden"
#define LC_CONFIGURE_LOADED LC_CONFIGURE_PREFIX("{} Einstellungen geladen.")
#define LC_CONFIGURE_CANNOT_LOAD LC_CONFIGURE_PREFIX("kh.conf kann nicht geöffnet werden. Datei im Serververzeichnis nicht gefunden.")
#define LC_CONFIGURE_DSL_LOADING LC_CONFIGURE_PREFIX("AI DSL wird neu geladen...")
#define LC_CONFIGURE_DSL_LOADED LC_CONFIGURE_PREFIX("AI DSL neu geladen.")
#define LC_SAVE_PREFIX(x) "SAVE: " x
#define LC_LOAD_PREFIX(x) "LOAD: " x
#define LC_DELETE_PREFIX(x) "DELETE: " x
#define LC_COMMAND_PREFIX(x) "COMMAND: " x
#define LC_SAVE_CMD_NO_NAME LC_DELETE_PREFIX("Kein Speichername angegeben.\nVerwendung: delete <speichername>")
#define LC_SAVE_CMD_NO_SAVED_GAME LC_DELETE_PREFIX("Kein gespeichertes Spiel '{}' gefunden.")
#define LC_SAVE_CMD_DELETED_GAME LC_DELETE_PREFIX("Spiel '{}' gelöscht.")
#define LC_SAVE_CMD_HINT "Verwendung: save <NAME> - aktuelles Spiel unter dem Namen NAME speichern"
#define LC_SAVE_FAILED LC_SAVE_PREFIX("Spielzustand konnte nicht gesichert werden.")
#define LC_SAVE_UPDATE_TARGET_SAVED LC_SAVE_PREFIX("'{}' aktualisiert (Runde {}).")
#define LC_SAVE_NEW_TARGET_SAVED LC_SAVE_PREFIX("'{}' gespeichert (Runde {}).")
#define LC_LOAD_NOT_FOUND LC_LOAD_PREFIX("Keine gespeicherten Spiele gefunden.")
#define LC_LOAD_LIST_BANNER LC_LOAD_PREFIX("Deine gespeicherten Spiele")
#define LC_LOAD_INHIBITED LC_LOAD_PREFIX("Nur während der Phase BUILD_SHIPS erlaubt.")
#define LC_LOAD_TARGET_NOT_FOUND LC_LOAD_PREFIX("Kein gespeichertes Spiel namens '{}' gefunden.")
#define LC_LOAD_OVERWRITE LC_LOAD_PREFIX("Du bist bereits in diesem Spiel.")
#define LC_LOAD_TARGET_SUCCEEDED "Spiel '{}' geladen. Runde {} wird fortgesetzt."
#define LC_LOAD_TARGET_REQUESTED LC_LOAD_PREFIX("Eine Ladeanforderung für '{}' ist ausstehend.\nGib zuerst 'reject {}' ein.")
#define LC_LOAD_TARGET_WAIT LC_LOAD_PREFIX("Anforderung zum Laden von '{}' (Runde {}) gestellt.\nAuf Zustimmung des anderen Spielers warten.")
#define LC_LOAD_TARGET_PROPOSAL LC_COMMAND_PREFIX("{} schlägt vor, '{}' (Runde {}) zu laden.\nGib 'accept {}' oder 'reject {}' ein")
#define LC_LOAD_NO_PENDING "Keine ausstehende Ladeanforderung."
#define LC_LOAD_TARGET_PENDING "Ausstehende Anforderung betrifft '{}', nicht '{}'"
#define LC_LOAD_WAIT_FOR_ACCEPT "Warten auf Zustimmung des anderen Spielers."
#define LC_LOAD_TARGET_LOAD_CONFIRMED "[LOAD] Zustimmung bestätigt, wechsle zu Spiel {}"
#define LC_LOAD_TARGET_LOADED LC_COMMAND_PREFIX("Spiel '{}' geladen. Runde {} wird fortgesetzt.\n{}")
#define LC_LOAD_NO_PENDING_REQ "Keine ausstehende Ladeanforderung."
#define LC_LOAD_TARGET_REJECT_NOT_SAME_REQ "Ausstehende Anforderung betrifft '{}', nicht '{}'"
#define LC_LOAD_TARGET_REJECT_REQ "Ladeanforderung für '{}' abgelehnt."
#define LC_LOAD_TARGET_REJECTED_RESPONSE LC_COMMAND_PREFIX("{} hat das Laden von '{}' abgelehnt")
#define LC_RECAP_IN_PROGRESS "Das Spiel läuft noch."
#define LC_RECAP_FLAVOR_1 "\"Die Sterne erinnern sich an jene, die es wagten.\""
#define LC_RECAP_FLAVOR_2 "\"Per aspera ad astra.\""
#define LC_RECAP_FLAVOR_3 "\"Jenseits des Horizonts besteht das Licht weiter.\""
#define LC_RECAP_FLAVOR_4 "\"Was kartiert wurde, kann nicht mehr unbekannt sein.\""
#define LC_RECAP_FLAVOR_5 "\"Die Leere gibt ihre Geheimnisse den Beharrlichen preis.\""
#define LC_RECAP_FLAVOR_6 "\"Alle Reisen enden. Nur wenige werden erinnert.\""
#define LC_CRT_TABLE_BANNER "KAMPFERGEBNISTABELLE"
#define LC_CRT_SUFFIX_PHASIC_NOTE "Schaden = Phasische Kraft + Technikstufe"
#define LC_CRT_SUFFIX_HEX_NOTE ">> kann durch Feldereignisse beeinflusst werden <<"
#define LC_GAME_WINNER "SPIEL BEENDET - Sieger: {}"
#define LC_VP_TO_WIN "Sieg: {} zum Gewinnen"
#define LC_ADDED_TARGET_VP "{} VP für die Kontrolle des feindlichen Basissystems!"
#define LC_VP_SELF_WON_GAME  "*** SIEG! Du hast das Spiel gewonnen! ***"
#define LC_VP_SELF_LOST_GAME  "*** NIEDERLAGE. Dein Gegner hat gewonnen. ***"
#define LC_COMBAT_PREFIX(x) "TACTICAL: " x
#define LC_COMBAT_ERR_PREFIX(x) "TACTICAL ERROR: " x
#define LC_COMBAT_SHIP_PREFIX(x) LC_COMBAT_PREFIX("Schiff {} ") x
#define LC_COMBAT_SHIP_NOT_EXIST LC_COMBAT_SHIP_PREFIX("existiert nicht.")
#define LC_COMBAT_SHIP_DESTROYED LC_COMBAT_SHIP_PREFIX("wurde zerstört.")
#define LC_COMBAT_SHIP_NOT_YOURS LC_COMBAT_SHIP_PREFIX("gehört dir nicht.")
#define LC_COMBAT_SHIP_NOT_HOSTILE LC_COMBAT_SHIP_PREFIX("ist kein feindliches Ziel.")
#define LC_COMBAT_SHIP_NOT_DEPLOYED LC_COMBAT_SHIP_PREFIX("ist nicht eingesetzt.")
#define LC_COMBAT_INITIATIVE_PLAYER_ONLY LC_COMBAT_ERR_PREFIX("Nur der Initiativspieler darf den Kampf eröffnen.")
#define LC_COMBAT_NO_RED_FORCES LC_COMBAT_PREFIX("Keine feindlichen Kräfte in diesem Feld.")
#define LC_COMBAT_NO_ORDERS LC_COMBAT_ERR_PREFIX("Befehle werden nicht angenommen (Phase {})")
#define LC_COMBAT_NO_QUEUED_ORDERS LC_COMBAT_PREFIX("Keine Kampfbefehle in der Warteschlange.")
#define LC_COMBAT_PENDING_ORDER_ERR LC_COMBAT_ERR_PREFIX("Befehle ausstehend für Feld {}, aktiver Kampf befindet sich jedoch in Feld {}")
#define LC_COMBAT_ORDERS_TX LC_COMBAT_PREFIX("Befehle übermittelt.")
#define LC_COMBAT_ORDERS_NO_RX LC_COMBAT_PREFIX("Es wurden keine Kampfbefehle empfangen.")
#define LC_COMBAT_ORDERS_RECINDED LC_COMBAT_PREFIX("Befehle zurückgezogen. Neue Befehle erteilen.")
#define LC_COMBAT_NO_PENDING_ORDERS LC_COMBAT_PREFIX("Keine ausstehenden Kampfbefehle.")
#define LC_COMBAT_RED_FORCE_HAS_ORDERS LC_COMBAT_PREFIX("Feind hat Kampfbefehle für Feld {} festgelegt")
#define LC_COMBAT_WAIT_RED_FORCE_ORDERS LC_COMBAT_PREFIX("Sektor {} - Warte auf feindliche Befehle")
#define LC_COMBAT_TACTICAL_COMBAT_PENDING_STAT LC_COMBAT_PREFIX("Kampf ausstehend! {} aktive Gefechte müssen aufgelöst werden.")
#define LC_COMBAT_TACTICAL_HINT "Verwende: CO <schiff> <ziel> <taktik> [PD=X [P=Y [...]]"
#define LC_COMBAT_TACTICAL_TARGET_RETREAT_PENDING LC_COMBAT_PREFIX("Rückzug ausstehend! {} Schiff(e) müssen den Rückzug abschließen")
#define LC_COMBAT_TACTICAL_RETREAT_HINT "Verwende: retreat <schiff> <feld>"
#define LC_COMBAT_TACTICAL_GAME_OVER ">> SPIEL BEENDET <<"
#define LC_COMBAT_ROUND_TARGET_DMG_REPORT LC_COMBAT_PREFIX("RUNDE {} SCHADENSBERICHT\n" \
    "{} GESAMT: {} SCHADEN ZU VERTEILEN\n" \
    "Verwende 'combat apply <schiff> pd=X [P=Y] ...' um Schaden zuzuweisen.")
#define LC_COMBAT_ROUND_TARGET_NO_DMG LC_COMBAT_PREFIX("RUNDE {} - DEINE SCHIFFE HABEN KEINEN SCHADEN ERLITTEN.")
#define LC_COMBAT_TARGET_STALEMATE_MSG LC_COMBAT_PREFIX("Zwei aufeinanderfolgende Pattsituationen: Du musst deine Schiffe aus Feld {} zurückziehen")
#define LC_COMBAT_NEXT_ROUND LC_COMBAT_PREFIX("Kampfrunde {} beginnt in Feld {}. Befehle erteilen.")
#define LC_COMBAT_TARGET_DMG_ASSIGNED LC_COMBAT_PREFIX("{} hat allen Schaden zugeteilt.")
#define LC_COMBAT_TARGET_COMBAT_ENDED LC_COMBAT_PREFIX("Kampf in Feld {} beendet. {} kontrolliert das Feld.")
#define LC_COMBAT_TARGET_ALL_DMG_NEW_ORDERS LC_COMBAT_PREFIX("Gesamter Schaden zugeteilt. Kampfrunde {} beginnt in Feld {}. Befehle erteilen!")
#define LC_COMBAT_ENEMY_DETECTED LC_COMBAT_PREFIX("Feindliche Kräfte entdeckt. Kampfbefehle sind optional.")
#define LC_COMBAT_TARGET_CONFLICT_BANNER "KONFLIKT IM STERNSYSTEM: {} [{}]\n" "SCHIFFE IM SYSTEM {}"
#define LC_COMBAT_TARGET_CONFLICT_BLUEFORCE "Blaue Kräfte"
#define LC_COMBAT_TARGET_CONFLICT_REDFORCE "Blaue Kräfte"
#define LC_ROOM_TARGET_SELF_HAVE_INIT LC_COMMAND_PREFIX("DU HAST DIE INITIATIVE! {}")
#define LC_ROOM_TARGET_N_SELF_HAVE_INIT LC_COMMAND_PREFIX("{} HAT DIE INITIATIVE. BEREITHALTEN...")
#define LC_TURN_HAS_ENDED "Dein Zug ist beendet."
#define LC_TURN_TARGET_PLAYER_TURN_BEGUN "Der Zug von {} hat begonnen."
#define LC_TURN_TARGET_THIS_PLAYER_TURN_BEGUN "Dein Zug: {} Runde {}"
#define LC_TURN_TARGET_IT_IS_YOUR_TURN "Du bist dran! Phase: {}"
#define LC_TURN_DECLARE_PHASE "Phase {}"
#define LC_TURN_DECLARE_ROUND "Runde {}"
#define LC_TURN_OPP_LEFT_GAME "Gegner hat das Spiel verlassen. Spiel gespeichert und beendet."
#define LC_TURN_TARGET_AUTO_SAVE_TO_LOBBY "Spiel automatisch als '{}' gespeichert.\nRückkehr zur Lobby.\n\033[LOBBY]"
#define LC_TARGET_SHIP_NOT_FOUND "Schiff {} nicht gefunden"
#define LC_MILIEU_PREFIX(y, x) y ": " x
#define LC_MILIEU_FABRICATE "FABRICATE"
#define LC_MILIEU_SURVEY "SURVEY"
#define LC_MILIEU_SYSTEM "SYSTEM"
#define LC_MILIEU_OUTFIT "OUTFIT"
#define LC_MILIEU_SALVAGE "SALVAGE"
#define LC_MILIEU_MARKET "MARKET"
#define LC_MILIEU_EXTRACT "EXTRACT"
#define LC_MILIEU_UNKNOWN_FABRICATE_MODE LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Unbekannter Fertigungsmodus.")
#define LC_MILIEU_FABRICATE_HINT "Verwende: fabricate <plan> [menge]"
#define LC_MILIEU_FABRICATE_CARGO_CHK_FAILED LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Frachtinventar konnte nicht geprüft werden.")
#define LC_MILIEU_FABRICATE_TARGET_LACK_RES LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Unzureichend {}. Benötigt {}, vorhanden {}")
#define LC_MILIEU_FABRICATE_TARGET_NO_PLAN LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Plan '{}' nicht in der Datenbank gefunden.")
#define LC_MILIEU_FABRICATE_NO_SHIPS_DEPLOYED LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Keine Schiffe an Fertigungsstandorten eingesetzt.")
#define LC_MILIEU_FABRICATE_SHIP_LOCATION_INVALID LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Ein Schiff muss sich an einer von dir kontrollierten WERFT oder RAFFINERIE befinden.")
#define LC_MILIEU_FABRICATE_TARGET_RESULT LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "{} Torpedos auf {} geladen")
#define LC_MILIEU_FABRICATE_LAUNCHER_UPGRADE_TARGET_RESULT LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Werfer-Aufrüstung +{} in der Warteschlange, abgeschlossen in Runde {}")
#define LC_MILIEU_FABRICATE_PHASIC_UPGRADE_TARGET_RESULT LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Phasische Aufrüstung +{} in der Warteschlange, abgeschlossen in Runde {}")
#define LC_MILIEU_FABRICATE_SHIELD_UPGRADE_TARGET_RESULT LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Schild-Aufrüstung +{} in der Warteschlange, abgeschlossen in Runde {}")
#define LC_MILIEU_FABRICATE_TECH_UPGRADE_TARGET_RESULT LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "Technikforschung +{} in der Warteschlange, abgeschlossen in Runde {}")
#define LC_MILIEU_FABRICATE_TARGET_QUEUED_COMPLETE LC_MILIEU_PREFIX(LC_MILIEU_FABRICATE, "FERTIGUNG ABGESCHLOSSEN: {} - {}")
#define LC_MILIEU_NO_SHIPS_FOR_SURVEY LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "Keine Schiffe für die Durchführung einer Erkundung verfügbar.")
#define LC_MILIEU_SURVEY_UNKNOWN_SYSTEM LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "Unbekanntes System '{}'")
#define LC_MILIEU_SURVEY_NO_SHIPS_PRESENT LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "Keine Schiffe in {} vorhanden")
#define LC_MILIEU_SURVEY_SHIPS_REQUIRED "Du musst ein Schiff im System haben, um Erkundungsoperationen durchzuführen."
#define LC_MILIEU_SURVEY_MAX_INFO LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "{} bereits auf maximalem Kenntnisstand (Vertraut).")
#define LC_MILIEU_SURVEY_MAX_SECRETS "Alle Systemgeheimnisse sind dir bekannt."
#define LC_MILIEU_TARGET_SURVEY_COMPLETE LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "{} Erkundung abgeschlossen.")
#define LC_MILIEU_TARGET_INFO_UPGRADED LC_MILIEU_PREFIX(LC_MILIEU_SURVEY, "Kenntnisstand verbessert: {} -> {} Erkundung abgeschlossen.")
#define LC_MILIEU_SURVEY_CHARTED_LEVEL "Planetare Daten und Standorte von Einrichtungen jetzt verfügbar."
#define LC_MILIEU_SURVEY_DETAILED_LEVEL "Detaillierte Ressourcendaten jetzt verfügbar."
#define LC_MILIEU_SURVEY_INTIMATE_LEVEL "Alle Systemgeheimnisse enthüllt, einschließlich Anomalien."
#define LC_MILIEU_SURVEY_TARGET_MORE "Verwende 'system {} planets' zum Anzeigen."
#define LC_MILIEU_SURVEY_TARGET_RESOURCES "Verwende 'system {} resources' zum Anzeigen."
#define LC_MILIEU_SURVEY_TARGET_ANOMALIES "Verwende 'system {} anomalies' zum Anzeigen."
#define LC_MILIEU_UNKNOWN_SYSTEM LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Unbekanntes System '{}'")
#define LC_MILIEU_UNKNOWN_SYSTEM_SUBCOMMAND LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Unbekannter Unterbefehl. > HELP SYSTEM")
#define LC_MILIEU_SYSTEM_NO_SHIPS LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Du hast keine Schiffe in diesem System zur Erkundung.")
#define LC_MILIEU_SYSTEM_NO_POWER LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Unzureichend PD zur Erkundung. Benötigt {} PD.")
#define LC_MILIEU_SYSTEM_SHOW_BANNER LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Sternsystem: {}")
#define LC_MILIEU_SYSTEM_SHOW_FACILITIES_BANNER LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "{} EINRICHTUNGEN")
#define LC_MILIEU_SYSTEM_SHOW_INFO_LEVEL LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Kenntnisstand: {}")
#define LC_MILIEU_SYSTEM_SHOW_PLANET LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Planeten: {}")
#define LC_MILIEU_SYSTEM_SHOW_ASTEROID LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Asteroidengürtel: {}")
#define LC_MILIEU_SYSTEM_SHOW_TARGET_PLANET_HINT "Verwende 'system {} planets' für Planetendaten."
#define LC_MILIEU_SYSTEM_SHOW_TARGET_FACILITIES_HINT "Verwende 'system {} facilities' für Infrastruktur."
#define LC_MILIEU_SYSTEM_SHOW_TARGET_RESOURCES_HINT "Verwende 'system {} resources' für Vorkommen."
#define LC_MILIEU_SYSTEM_SHOW_TARGET_ANOMALIES_HINT "Verwende 'system {} anomalies' für Entdeckungen."
#define LC_MILIEU_SYSTEM_SHOW_TARGET_INFO_MIN LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, \
    "Planetare Daten erfordern den Kenntnisstand Kartiert.\nAktuell: {}. Ein Schiff zur Erkundung entsenden.")
#define LC_MILIEU_SYSTEM_NO_PLANET_RECORDS "Keine planetaren Körper verzeichnet."
#define LC_MILIEU_SYSTEM_NO_INFRA_RECORDS "Keine nennenswerte Infrastruktur verzeichnet."
#define LC_MILIEU_SYSTEM_SHOW_TARGET_INFO_MIN_SURVEY LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, \
    "Ressourcendaten erfordern den Kenntnisstand Erkundet.\nAktuell: {}. Verwende den Befehl 'survey'.")
#define LC_MILIEU_SYSTEM_FACILITIES_DATA_TARGET_INFO_MIN_CHARTED LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, \
    "Einrichtungsdaten erfordern den Kenntnisstand Kartiert.\nAktuell: {}. Ein Schiff zur Erkundung entsenden.")
#define LC_MILIEU_SYSTEM_ANOMALIES_DATA_TARGET_INFO_MIN_INTIMATE LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, \
    "Anomaliedaten erfordern den Kenntnisstand Vertraut.\nAktuell: {}. Längere Präsenz erforderlich.")
#define LC_MILIEU_SYSTEM_TARGET_SYSTEM_UNKNOWN LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Unbekanntes System '{}'")
#define LC_MILIEU_SYSTEM_NO_SHIPS_FOR_SURVEY LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Du hast keine Schiffe in diesem System zur Erkundung.")
#define LC_MILIEU_SYSTEM_LACK_CR_FOR_SURVEY LC_MILIEU_PREFIX(LC_MILIEU_SYSTEM, "Unzureichend PD zur Erkundung. Benötigt {} PD.")
#define LC_MILIEU_MARKET_SHOW_BANNER LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "MARKTBÖRSE (Runde {})")
#define LC_MILIEU_MARKET_TARGET_HUB_REVENUE LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Handelsknoteneinnahmen +{} CR diese Runde.")
#define LC_MILIEU_MARKET_TARGET_NO_HISTORY LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Keine Handelshistorie für {}.")
#define LC_MILIEU_MARKET_NO_HISTORY_HINT LC_MILIEU_PREFIX(LC_MILIEU_MARKET, \
    "Preise haben sich in diesem Spiel möglicherweise noch nicht verändert.\nVerwende 'trade list' um aktuelle Wechselkurse anzuzeigen.")
#define LC_MILIEU_EXTRACT_NO_DEPLOYED_SHIPS LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT, "Keine eingesetzten Schiffe zur Förderung verfügbar.")
#define LC_MILIEU_EXTRACT_BANNER "ABBAUBARE RESSOURCEN"
#define LC_MILIEU_TARGET_AT "{} bei {}: "
#define LC_MILIEU_TARGET_AT_PREFIX(x) "{} bei {}: " x
#define LC_MILIEU_TARGET_NO_EXTRACT_RESOURCES LC_MILIEU_TARGET_AT_PREFIX("Keine Ressourcen")
#define LC_MILIEU_EXTRACT_HINT_RESOURCE "Verwende: extract <schiff> <ressourcentyp>"
#define LC_MILIEU_EXTRACT_TARGET_SHIP_MISSING LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT, "Schiff {} für die Förderung nicht gefunden.")
#define LC_MILIEU_EXTRACT_SHIP_MUST_BE LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT, "Schiff muss in einem System eingesetzt sein, um zu fördern.")
#define LC_MILIEU_EXTRACT_TARGET_NO_DEPOSITS LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT, "Keine {}-Vorkommen im Sternsystem {} gefunden")
#define LC_MILIEU_EXTRACT_CARGO_FULL LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT, "Frachtraum von {} ist voll!")
#define LC_MILIEU_EXTRACT_TARGET_SUCCEEDED LC_MILIEU_PREFIX(LC_MILIEU_EXTRACT, "{} hat {} Einheiten {} aus {} ({}) gefördert")
#define LC_MILIEU_MARKET_TRADE_REQMT LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Handel erfordert ein Schiff an einer TRADE_HUB-Einrichtung.")
#define LC_MILIEU_MARKET_TRADE_BANNER "HANDELSWECHSELKURSE"
#define LC_MILIEU_MARKET_TRADE_TARGET_UNKNOWN_RESOURCE LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Unbekannter Ressourcentyp: {}")
#define LC_MILIEU_MARKET_TRADE_TARGET_LACKING_CREDIT LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Unzureichend Kredite. Benötigt {} CR, vorhanden {} CR")
#define LC_MILIEU_MARKET_TRADE_TARGET_LACKING_RESOURCE LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Unzureichend {}. Vorhanden {}, benötigt {}")
#define LC_MILIEU_MARKET_TRADE_TARGET_SUCCEEDED LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "{} {} für {} CR gekauft.\nAuf {} geladen")
#define LC_MILIEU_MARKET_TRADE_NO_CARGO_SHIP LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Keine Schiffe zur Frachtaufnahme verfügbar.")
#define LC_MILIEU_MARKET_TRADE_TARGET_SOLD LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "{} {} für {} CR verkauft")
#define LC_MILIEU_MARKET_TRADE_TARGET_SHIP_LACKING_RESOURCE LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "{} hat nur {} {}")
#define LC_MILIEU_MARKET_TRADE_TARGET_XFR LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "{} {} von {} nach {} transferiert")
#define LC_MILIEU_MARKET_TRADE_SHIP_MISSING LC_MILIEU_PREFIX(LC_MILIEU_MARKET, "Schiff {} nicht gefunden.")
#define LC_MILIEU_OUTFIT_BANNER "AUSRÜSTUNGSAUSSTATTUNG"
#define LC_MILIEU_OUTFIT_REQMT "Erfordert ein Schiff an einer kontrollierten WERFT."
#define LC_MILIEU_OUTFIT_USAGE "Verwende: outfit <schiff> <ausrüstung>"
#define LC_MILIEU_OUTFIT_EXAMPLE "Beispiel: outfit W1 lrs"
#define LC_MILIEU_OUTFIT_TARGET_NOT_IN_CATALOG LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "Ausrüstung {} nicht im Katalog.")
#define LC_MILIEU_OUTFIT_TARGET_SHIP_NOT_FOUND LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, LC_TARGET_SHIP_NOT_FOUND)
#define LC_MILIEU_OUTFIT_TARGET_SHIP_NOT_DEPLOYED LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "Schiff {} ist nicht eingesetzt.")
#define LC_MILIEU_OUTFIT_TARGET_SHIP_NOT_AT_SYSTEM LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "Schiff {} befindet sich nicht in einem Sternsystem.")
#define LC_MILIEU_OUTFIT_TARGET_NO_SHIPYARD LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "{} hat keine von dir kontrollierte WERFT.")
#define LC_MILIEU_OUTFIT_LACK_CREDIT LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "Unzureichend Kredite. Benötigt {} CR, vorhanden {} CR.")
#define LC_MILIEU_OUTFIT_INSTALLED LC_MILIEU_PREFIX(LC_MILIEU_OUTFIT, "{} auf {} für {} CR installiert.")
#define LC_SALVAGE_EXAMPLE "Verwende: salvage <schiff> <ressource> zum Bergen."
#define LC_SALVAGE_NO_SHIP_CODE "Bergung ohne Schiffscode nicht möglich."
#define LC_SALVAGE_NO_SHIPS "BERGUNGSSCAN: Keine eingesetzten Schiffe zum Scannen."
#define LC_SALVAGE_UNKNOWN_SIGNAL "[?] Unbekanntes Signal entdeckt (nochmals scannen?)"
#define LC_SALVAGE_NONE "Keine Bergungsobjekte gefunden."
#define LC_SALVAGE_FOUND "{} Bergungsobjekt(e) bekannt."
#define LC_SALVAGE_SHIP_NOT_FOUND LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "Schiff {} nicht gefunden.")
#define LC_SALVAGE_NEED_DEPLOYED_SHIP LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "Schiff muss eingesetzt sein, um zu bergen.")
#define LC_SALVAGE_TARGET_NOT_FOUND LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "Keine bekannten Bergungsobjekte in {}.")
#define LC_SALVAGE_SUGGEST_SCAN "Verwende 'salvage scan' zum Suchen."
#define LC_SALVAGE_SUGGEST_TARGET_SALVAGE "Verwende: salvage {} <ressource>"
#define LC_SALVAGE_TARGET_AVAILABLE LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "Verfügbare Bergungsobjekte in {}")
#define LC_SALVAGE_NO_MATCH_TARGET LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "Kein Bergungsobjekt '{}' in {} gefunden")
#define LC_SALVAGE_TARGET_HAZARD LC_MILIEU_PREFIX(LC_MILIEU_SALVAGE, "{} ist bei {} auf Gefahren gestoßen. {} Rumpfschaden erlitten.\n")
#define LC_SALVAGE_SHIP_DESTROYED "Schiff {} zerstört!"
#define LC_SALVAGE_TARGET_DEPLEATED "{} ist nun erschöpft."
#define LC_REPRES_NO_DAMAGED_SHIPS_AT_BASES "Keine beschädigten Schiffe an deinen Basissternensystemen."
#define LC_REPRES_LIST_BANNER "Reparierbare Schiffe an deinen Basissternen"
#define LC_REPRES_HINT "Verwende: repair <schiffscode> <attribut>=<menge>"
#define LC_REPRES_EXAMPLE "Beispiel: repair W1 pd=2 (kostet 2 BP)"
#define LC_REPRES_TARGET_SHIP_NOT_FOUND "Schiff {} nicht gefunden."
#define LC_REPRES_SHIP_MUST_BE_AT_FACILITY "Schiff muss an deinem Basisstern oder einer kontrollierten Reparatureinrichtung sein."
#define LC_REPRES_TARGET_SPECIFY "Attribut und Menge angeben: repair {} pd=N [p=N [ ...]]  Siehe Hilfethema: Attribute"
#define LC_REPRES_INVALID_ATTR "Ungültiges Attribut. Siehe Hilfethema: Attribute"
#define LC_REPRES_AT_MAX "Attribut bereits auf Maximum."
#define LC_REPRES_REPAIR_TORPEDO_AT_MAX "Schiff bereits auf maximaler Torpedobestückung."
#define LC_REPRES_COST_LIMIT "Unzureichend CR. Benötigt {}, vorhanden {}"
#define LC_REPRES_TARGET_SUCCEEDED "{} {} um {} repariert (Kosten: {}) CR"
#define LC_REPRES_NO_TORPEDO_NEEDED "Keine Schiffe benötigen Torpedo-Nachschub an deinen Basissternensystemen."
#define LC_REPRES_RESUP_LIST_BANNER "Schiffe, die an deinen Basissternensystemen nachversorgt werden können"
#define LC_REPRES_RESUPPLY_HINT "Verwende: resupply <schiffscode> <menge>"
#define LC_REPRES_RESUPPLY_EXAMPLE "Beispiel: resupply W1 6 (kostet 2 CR für 6 Torpedos)"
#define LC_REPRES_RESUPPLY_SHIP_NOT_AT_BASE "Schiff nicht gefunden oder nicht an deinen Basissternensystemen"
#define LC_REPRES_RESUPPLY_TARGET_SPECIFY_TORPEDO "Menge angeben: resupply {} N"
#define LC_REPRES_TARGET_RESUPPLY_COST_LIMIT "Unzureichend CR. Benötigt {}, vorhanden {} CR"
#define LC_REPRES_TARGET_RESUPPLY_SUCCEEDED "{} mit {} Torpedo(s) nachversorgt (Kosten: {} CR)"
#define LC_HEX_INFO_TARGET_NO_SYSTEM "HEX: Kein System bei '{}' gefunden"
#define LC_HEX_INFO_TARGET_NO_SHIP_AT "HEX: Du hast keine Schiffe an diesem Ort ({}) für einen Scan."
#define LC_HEX_INFO_TARGET_LACK_CR "HEX: Unzureichend PD für den Scan. Benötigt {} PD."
#define LC_HEX_INFO_SCAN_TARGET_VIA "(LRS-Scan über {})"
#define LC_HEX_INFO_SCAN_COST_TARGET "(Scankosten: {} PD von {})"
#define LC_SS_H "Werftbericht: SystemShips können keine Hangars haben"
#define LC_TORPEDO_BY_THREE "Fehler: Torpedos müssen ein Vielfaches von 3 sein."
#define LC_NEG_ATTR "Fehler: Negative Attributwerte sind nicht erlaubt."
#define LC_TORPEDO_LAUNCHER_MISMATCH "Werfer müssen > 0 sein, wenn Torpedos > 0"
#define LC_INSUFFICIENT_BUILD_CREDITS "SHIPYARD: Unzureichend BP. Bau eingestellt."
#define LC_INVALID_HULL_DESIGNATION "SHIPYARD: Ungültige Rumpfbezeichnung. Schiffscode prüfen."
#define LC_DUPLICATE_HULL_CODE "SHIPYARD: Rumpf {} bereits auf dem Reißbrett."
#define LC_SHIP_ALREADY_COMISSIONED "SHIPYARD: Schiff {} bereits in der Flotte in Dienst gestellt."
#define LC_SHIP_HULL_LAID "SHIPYARD: Kiel {} gelegt. Bezeichnung: {}"
#define LC_NEED_HULL_DESIGNATOR_TO_FIND "SHIPYARD: Rumpfbezeichner oder Name des Schiffs wird benötigt."
#define LC_SHIPYARD_SHIP_NOT_EXIST "SHIPYARD: Schiff {} existiert nicht."
#define LC_SHIPYARD_LOST_SHIP "SHIPYARD: Schiff {} ist aus dem Trockendock VERSCHWUNDEN."
#define LC_SHIPYARD_SHIP_NOT_FOUND "SHIPYARD: Schiff mit Rumpfbezeichnung {} nicht in der Werft"
#define LC_SHIPYARD_HULL_CANCELED "SHIPYARD: Abgebrochen: {}"
#define LC_FLEET_OPS_NO_SHIPS "FLEET OPS: Keine Schiffe unter deinem Kommando."
#define LC_MOVE_TARGET_SHIP_NOT_IN_FLEET "FLEET REGISTRY: Schiff {} ist nicht in deiner Flotte!"
#define LC_MOVE_ONLY_W_SHIP_CAN_CHART "CHART: Nur WarpShip class Schiffe können Warpkurse kartieren."
#define LC_MOVE_TARGET_SHIP_HAS_NO_WARP_CAP "CHART: {} hat keinen Antrieb."
#define LC_MOVE_TARGET_SHIP_NOT_DEPLOYED "CHART: {} ist nicht eingesetzt."
#define LC_MOVE_TARGET_MOVEMENT_COST_PREAMBLE "Kosten: {} PD (verfügbar: {})"
#define LC_MOVE_TARGET_WARPLINE_JUMP "{} Warplinie {}"
#define LC_MOVE_TARGET_SHIP_IN_HANGAR_ERROR "Schiff ist eingerückt. Vor dem Einsetzen ausrücken: {}"
#define LC_MOVE_DEPLOY_UNKNOWN_SYSTEM "DEPLOY: Unbekanntes System '{}'"
#define LC_MOVE_DEPLOY_ONLY_AT_BASES "DEPLOY: Schiffe können nur an einem heimischen Basissternensystem eingesetzt werden."
#define LC_MOVE_TARGET_DEPLOY_ANNOUNCE_BASE_CLAIM "DEPLOY: {} hat {} beansprucht."
#define LC_MOVE_DEPLOY_INHIBITED_IN_ENEMY_TERRITORY "DEPLOY: Einsatz in feindlichem Gebiet nicht möglich. Heimische Basen verwenden."
#define LC_MOVE_TARGET_ANNOUNCE_DEPLOY "FLEET COMMAND: {} ({}) nach {} eingesetzt"
#define LC_MOVE_INHIBITED_WARP "NAV: Nur WarpShip class Schiffe können den Hyperantrieb aktivieren."
#define LC_MOVE_TARGET_INHIBITED_WARP "NAV: {} hat keinen Antrieb. Manöver nicht möglich."
#define LC_MOVE_TARGET_SHIP_IN_HANGAR "OPS: Schiff {} ist eingerückt und kann sich nicht bewegen"
#define LC_MOVE_TARGET_IN_SHIPYARD_INHIBITED "NAV: {} ist nicht eingesetzt. Noch in der Werft."
#define LC_MOVE_TARGET_AT_DEST "NAV: {} befindet sich bereits bei {}."
#define LC_MOVE_TARGET_POWERDRIVE_DEPLEATED "NAV: {} hat den Antrieb für diesen Zug aufgebraucht."
#define LC_MOVE_TARGET_HAZARD_ON_MOVE "HAZARD: Navigationsgefahr in Feld {}!\n{} PD verbraucht, um die Gefahr zu überwinden."
#define LC_MOVE_COURSE_PLOT_BANNER "NAV: Kurs berechnet. Bereithalten."
#define LC_MOVE_TARGET_TRANSIT_PATH "OPS: {} ({}) Transit nach {} [{}] abgeschlossen.\nVerbrauchte Energie: {} PD"
#define LC_MOVE_TARGET_TRIGGER_TACTICAL "TACTICAL ALERT: Kontakt!\nFeindliche Kräfte in {} entdeckt!"
#define LC_MOVE_TARGET_TRIGGER_ENEMY_IN_RANGE LC_MOVE_TARGET_TRIGGER_TACTICAL "\nEin Angriff während der Kampfphase ist möglich."
#define LC_MOVE_TACTICAL_ALERT_BANNER "TACTICAL ALERT"
#define LC_MOVE_TACTICAL_ALERT_DETECTED "SENSOREN ENTDECKEN"
#define LC_RETREAT_TARGET_SHIP_NOT_FOUND "RETREAT: Schiff {} nicht gefunden"
#define LC_RETREAT_TARGET_NO_PENDING_RETREAT_REQMT "RETREAT: {} hat keinen ausstehenden Rückzug."
#define LC_RETREAT_TARGET_INVALID_HEX "RETREAT: Ungültiges Feld: {}"
#define LC_RETREAT_TARGET_INVALID_PROXIMITY "RETREAT: {} grenzt nicht an {}"
#define LC_RETREAT_VALID_DEST_BANNER "Gültige Rückzugsziele"
#define LC_RETREAT_TARGET_ANNOUNCE "RETREAT: {} zieht sich nach {} zurück"
#define LC_PICKDROP_PREFIX(x) "OPS: " x
#define LC_PICKDROP_TARGET_NO_SHIP_IN_FLEET LC_PICKDROP_PREFIX("SystemShip {} nicht in deiner Flotte gefunden.")
#define LC_PICKDROP_TARGET_INHIBIT_WARPSHIP LC_PICKDROP_PREFIX("{} ist ein WarpShip. Ein WarpShip kann nicht eingerückt werden.")
#define LC_PICKDROP_TARGET_ALREADY_HANGAR LC_PICKDROP_PREFIX("{} ist bereits im SystemShip-Hangar von {} gesichert")
#define LC_PICKDROP_TARGET_NO_WARPSHIP_IN_FLEET LC_PICKDROP_PREFIX("WarpShip {} nicht in deiner Flotte gefunden.")
#define LC_PICKDROP_TARGET_NOT_WARPSHIP LC_PICKDROP_PREFIX("{} ist kein WarpShip.")
#define LC_PICKDROP_TARGET_WARPSHIP_LACK_HANGAR LC_PICKDROP_PREFIX("{} hat keine SystemShip-Hangars (H=0).")
#define LC_PICKDROP_TARGET_WARPSHIP_LOCATION LC_PICKDROP_PREFIX("Schiffe müssen am selben Ort sein. {} bei {}, {} bei {}")
#define LC_PICKDROP_TARGET_WARPSHIP_HANGAR_CAPACITY LC_PICKDROP_PREFIX("{} SystemShip-Hangars sind voll ({}/{}).")
#define LC_PICKDROP_TARGET_SYSTEMSHIP_HANGAR_SUCCESS LC_PICKDROP_PREFIX("{} an Bord von {} aufgenommen.")
#define LC_PICKDROP_TARGET_SYSTEMSHIP_NOT_IN_HANGAR LC_PICKDROP_PREFIX("SystemShip {} befindet sich in keinem WarpShip.")
#define LC_PICKDROP_TARGET_NO_WARP_SHIP_IN_FLEET LC_PICKDROP_PREFIX("WarpShip {} nicht in deiner Flotte gefunden.")
#define LC_PICKDROP_TARGET_SYSTEMSHIP_HELD_DIFFERENT LC_PICKDROP_PREFIX("{} befindet sich nicht in {}. (Derzeit bei {} aufgenommen).")
#define LC_PICKDROP_TARGET_INHIBIT_DROP_SPACE LC_PICKDROP_PREFIX("SystemShip kann nicht im freien Raum ausgerückt werden. {} muss sich in einem Sternsystem befinden.")
#define LC_PICKDROP_TARGET_SYSTEMSHIP_DROP_SUCCESS LC_PICKDROP_PREFIX("{} von {} bei {} eingesetzt.")
#define LC_DEMO_PAGE_INFO                                                          \
    "DEMONSTRATION\n"                                                              \
    "BAU\n"                                                                        \
    " > BN W IRONSTAR\n"                                                           \
    " > BS PD=8 P=4 S=1 L=1 T=3 H=1\n"                                            \
    " > BC\n"                                                                      \
    "BEWEGUNG\n"                                                                   \
    "  Ein Wegpunkt kann ein HEX oder ein benanntes Sternsystem sein\n"            \
    "  Wenn W4 von HAVOR nach LYRIS nach NOREX nach H1613 will\n"                  \
    " > M W4 LYRIS NOREX H1613\n"                                                  \
    "KAMPF\n"                                                                      \
    " > CO W2 A W9 PD=5 P=3\n"                                                    \
    "  W2 setzt Angriffstaktik gegen W9 ein.\n"                                    \
    "  W2 fährt Antrieb auf Stufe 5 hoch\n"                                        \
    "  W2 aktiviert Phasik auf Stufe 3\n"                                          \
    " Als Verteidiger (Besitzer von W9) eigene Befehle eingeben:\n"                \
    " > CO W9 D W2 PD=4 P=1\n"                                                    \
    "  W9 setzt Ausweichtaktik gegen W2 ein.\n"                                    \
    "  W9 fährt Antrieb auf Stufe 4 hoch\n"                                        \
    "  W9 aktiviert Phasik auf Stufe 1\n"
#define LC_HELP_PAGE_INFO_BASIC                                                    \
    "TAKTISCHE BEFEHLE\n"                                                          \
    "BAU\n"                                                                        \
    "  BN W/S {NAME}          Neues WarpShip/SystemShip NAME bauen\n"              \
    "  BS {SCHIFF} {SPEZ}     Schiffsattribute festlegen\n"                        \
    "  BC/BX                  In Flotte übergeben / Entwurf verwerfen\n"           \
    "  BD                     Entwürfe anzeigen\n"                                 \
    "BEWEGUNG\n"                                                                   \
    "  M {SCHIFF} {LISTE}     Schiff zu/durch System(e) oder Feld(er) bewegen\n"   \
    "  DEPLOY {SCHIFF} {SYS}  SCHIFF in SYS einsetzen\n"                           \
    "  PICK/DROP {SCHIFF}     SystemShip einrücken/ausrücken beim WarpShip\n"      \
    "KAMPF\n"                                                                      \
    "  C                      Aktiven Kampf anzeigen\n"                            \
    "  CO {SCHIFF} A/D/E {ZL} Eigenes SCHIFF greift an/verteidigt/weicht ZL aus\n" \
    "  CC/CX                  Befehle ausführen/abbrechen\n"                       \
    "  CD {SCHIFF} {SPEZ}     Schaden dem Schiff zuweisen\n"                       \
    "ZUG\n"                                                                        \
    "  N/FF                   Nächste Phase / Vorspulen (beendet Zug)\n"           \
    "INFORMATION\n"                                                                \
    "  FLEET | SCORE | STATUS | SYSTEM {NAME} | SURVEY | CRT | DEMO\n"
#define LC_HELP_NO_TOPICS "Keine Themen verfügbar."
#define LC_HELP_TOPICS_BANNER "VERFÜGBARE HILFETHEMEN"
#define LC_HELP_TARGET_NO_TOPIC "Keine Hilfe verfügbar für '{}'. Verwende: help topics"

// Parser
#define LC_PARSER_YY_SAVE_RESERVED_WORDS "save: Verwendung: save <name>. Hinweis: reservierte Wörter können nicht als Namen verwendet werden."
#define LC_PARSER_YY_LOAD "load: Verwendung: load <name>."
#define LC_PARSER_YY_ACCEPT "accept: Verwendung: accept <name>."
#define LC_PARSER_YY_REJECT "reject: Verwendung: reject <name>."
#define LC_PARSER_YY_DELETE "delete: Verwendung: delete <name>."
#define LC_PARSER_YY_FLEET_TOO_MANY_ARGS "Zu viele Argumente. Verwendung: fleet > HELP FLEET"
#define LC_PARSER_YY_FLEET_NO_ARGS "fleet benötigt keine Argumente. Verwendung: fleet > HELP FLEET"
#define LC_PARSER_YY_HEX "hex: Verwendung: hex <feld_id>"
#define LC_PARSER_YY_SYSTEM "system: Verwendung: system <name> [unterbefehl]"
#define LC_PARSER_YY_SURVEY "survey: Verwendung: survey [systemname]"
#define LC_PARSER_YY_EXTRACT "extract: Verwendung: extract scan | extract <schiff> <ressource> [ressourcenwörter...]"
#define LC_PARSER_YY_MARKET "market: Verwendung: market [systemname]"
#define LC_PARSER_YY_TRADE "trade: Verwendung: trade buy|sell|transfer ..."
#define LC_PARSER_YY_FABRICATE "fabricate: Verwendung: fabricate <schiff> <gegenstand>"
#define LC_PARSER_YY_OUTFIT "outfit: Verwendung: outfit <schiff> <spezifikation>"
#define LC_PARSER_YY_SALVAGE "salvage: Verwendung: salvage <schiff> <ziel>"
#define LC_PARSER_YY_CARGO "cargo: Verwendung: cargo <schiff> [ressourcenwörter...]"
#define LC_PARSER_YY_SCAN "scan: Verwendung hängt vom Befehl ab (z.B. extract scan)"
#define LC_PARSER_YY_LIST "list: Verwendung hängt vom Kontext ab"
#define LC_PARSER_YY_NEXT "next: Verwendung: next"
#define LC_PARSER_YY_DONE "done: Verwendung: done"
#define LC_PARSER_YY_CD "cd: Verwendung: cd"
#define LC_PARSER_YY_CO "co: Verwendung: co <angreifer> <attack|dodge|escape> <ziel?> [PD=n P=n S=n L=n T=n H=n]"
#define LC_PARSER_YY_CA "ca: Verwendung: ca <entwurf_id> <schadensangabe...>"
#define LC_PARSER_YY_CC "cc: Verwendung: cc <entwurf_id>"
#define LC_PARSER_YY_CX "cx: Verwendung: cx <entwurf_id>"
#define LC_PARSER_YY_DUPE_POWER_DRIVE "doppelte POWER_DRIVE-Zuweisung"
#define LC_PARSER_YY_DUPE_PHASIC "doppelte PHASIC-Zuweisung"
#define LC_PARSER_YY_DUPE_SHIELD "doppelte SHIELD-Zuweisung"
#define LC_PARSER_YY_DUPE_LAUNCHER "doppelte LAUNCHER-Zuweisung"
#define LC_PARSER_YY_SHIP_CLASS_EMPTY "Ungültige Schiffsklasse: leere Zeichenkette > HELP BN"
#define LC_PARSER_YY_SHIP_CLASS_INVALID "Ungültige Schiffsklasse: {}. Muss mit 'W' oder 'S' beginnen."
#define LC_PARSER_YY_SHIP_CLASS_FORMAT "Ungültiges benutzerdefiniertes Schiffsklassen-ID-Format: '{}'. Zahl muss 1-3 Stellen haben."
#define LC_PARSER_YY_ATTR_INVALID_PD "Ungültiger Wert für PD: '{}'. Muss eine positive ganze Zahl sein. > HELP ATTR"
#define LC_PARSER_YY_ATTR_INVALID_P "Ungültiger Wert für P: '{}'. Muss eine positive ganze Zahl sein. > HELP ATTR"
#define LC_PARSER_YY_ATTR_INVALID_S "Ungültiger Wert für S: '{}'. Muss eine positive ganze Zahl sein. > HELP ATTR"
#define LC_PARSER_YY_ATTR_INVALID_L "Ungültiger Wert für L: '{}'. Muss eine positive ganze Zahl sein. > HELP ATTR"
#define LC_PARSER_YY_ATTR_INVALID_T "Ungültiger Wert für T: '{}'. Muss eine positive ganze Zahl sein. > HELP ATTR"
#define LC_PARSER_YY_ATTR_INVALID_H "Ungültiger Wert für H: '{}'. Muss eine positive ganze Zahl sein. > HELP ATTR"
#define LC_PARSER_YY_BN_TOO_MANY_ARGS "Zu viele Argumente. Verwendung: bn {W|S} name > HELP BN"
#define LC_PARSER_YY_BN_MISSING_NAME "Schiffsname fehlt. Verwendung: bn {W|S} name > HELP BN"
#define LC_PARSER_YY_BN_MISSING_ARGS "Argumente fehlen. Verwendung: bn {W|S} name > HELP BN"
#define LC_PARSER_YY_BUILD_NEW_TOO_MANY_ARGS "Zu viele Argumente. Verwendung: build new {W|S} name > HELP BUILD"
#define LC_PARSER_YY_BUILD_NEW_MISSING_NAME "Schiffsname fehlt. Verwendung: build new {W|S} name > HELP BUILD"
#define LC_PARSER_YY_BUILD_NEW_MISSING_ARGS "Argumente fehlen. Verwendung: build new {W|S} name > HELP BUILD"
#define LC_PARSER_YY_BUILD_SET_MISSING_ATTRS "Attribute fehlen. Verwendung: build set {NAME|RUMPF} attr=# ... > HELP BS"
#define LC_PARSER_YY_BUILD_UNEXPECTED_TOKENS "Unerwartete Token nach Attributen. > HELP BS"
#define LC_PARSER_YY_BUILD_SET_MISSING_ALL "Schiffsname/Rumpf und Attribute fehlen. Verwendung: build set {NAME|RUMPF} attr=# ... > HELP BS"
#define LC_PARSER_YY_BS_MISSING_ATTRS "Attribute fehlen. Verwendung: bs {NAME|RUMPF} attr=# ... > HELP BS"
#define LC_PARSER_YY_BS_MISSING_ALL "Schiffsname/Rumpf und Attribute fehlen. Verwendung: bs {NAME|RUMPF} attr=# ... > HELP BS"
#define LC_PARSER_YY_BUILD_SET_INVALID_ATTR "Ungültiges Attribut: {}. Gültige Attribute: PD, P, S, L, T, H > HELP ATTR"
#define LC_PARSER_YY_BS_INVALID_ATTR "Ungültiges Attribut: {}. Gültige Attribute: PD, P, S, L, T, H > HELP BS"
#define LC_PARSER_YY_BUILD_SYNTAX "build-Syntax               > HELP BUILD"
#define LC_PARSER_YY_BD "Verwendung: bd {NAME|RUMPF}      > HELP BD"
#define LC_PARSER_YY_BC "Verwendung: bc {NAME|RUMPF}      > HELP BC"
#define LC_PARSER_YY_BX "Verwendung: bx {NAME|RUMPF}      > HELP BX"
#define LC_PARSER_YY_DEPLOY "deploy: Verwendung: deploy <schiff> <ort>"
#define LC_PARSER_YY_MOVE "move: Verwendung: move <schiff> <ziel> [über ...]"
#define LC_PARSER_YY_CHART "chart: Verwendung: chart <schiff> <ziel> [über ...]"
#define LC_PARSER_YY_PICK "pick: Verwendung: pick <systemship> <warpship>"
#define LC_PARSER_YY_DROP "drop: Verwendung: drop <systemship> <warpship>"
#define LC_PARSER_YY_REPAIR "repair: Verwendung: repair [<schiff> PD=n|P=n|S=n|L=n]"
#define LC_PARSER_YY_RESUPPLY "resupply: Verwendung: resupply <schiff> T=n"
#define LC_PARSER_YY_RETREAT "retreat: Verwendung: retreat <schiff> <ziel>"
#define LC_PARSER_YY_REPAIR_NO_TORPEDOES "repair: Torpedos können nicht repariert werden. resupply verwenden."
#define LC_PARSER_YY_REPAIR_NO_HANGAR "repair: Hangar ist kein reparierbares Attribut."
#define LC_PARSER_YY_CONFIGURE "configure: Verwendung: configure [reload conf|reload ai]"
#define LC_PARSER_YY_HELP "help: Verwendung: help [thema]."

#endif
