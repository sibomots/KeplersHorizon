///////////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
// 
// This file is part of Kepler's Horizon
//
// Copyright (c) 2025, sibomots
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// behavior.js (client-side behavior layer)
// Exports window.KEPLERHORIZON.behavior for interface.js to call.

(function () {
  "use strict";

  window.KEPLERHORIZON = window.KEPLERHORIZON || {};
  const C = window.KEPLERHORIZON.constraints;
  const S = window.KEPLERHORIZON.slate;

  function $(id) { return document.getElementById(id); }

  function setText(id, value) {
    const el = $(id);
    if (!el) return;
    el.textContent = value;
  }

  function apiUrl(path) {
    const base = (C && C.apiBase) ? C.apiBase : "/kh/api";
    const p = path.startsWith("/") ? path : ("/" + path);
    return base + p;
  }

  function authHeaders() {
    const h = { "Content-Type": "application/json" };
    if (S && S.token) h["Authorization"] = "Bearer " + S.token;
    return h;
  }

  async function apiJson(path, method, bodyObj, needsAuth) {
    const opts = { method: method, headers: needsAuth ? authHeaders() : { "Content-Type": "application/json" } };
    if (bodyObj !== undefined && bodyObj !== null) opts.body = JSON.stringify(bodyObj);

    const r = await fetch(apiUrl(path), opts);
    const j = await r.json().catch(() => ({}));
    if (!j || j.ok !== true) {
      const msg = (j && j.error) ? j.error : ("server error (" + r.status + ")");
      throw new Error(msg);
    }
    return j;
  }

  function appendLine(text, cls) {
    const log = $("consoleLog");
    if (!log) {
      return;
    }
    const div = document.createElement("div");
    div.className = cls || "";
    div.textContent = text;
    log.appendChild(div);
    log.scrollTop = log.scrollHeight;
  }

  function setLoginBadge() {
    const b = $("loginBadge");
    if (!b) return;
    if (S && S.token) {
      b.textContent = "LOGGED IN";
      b.classList.remove("bad");
      b.classList.add("good");
    } else {
      b.textContent = "LOGGED OUT";
      b.classList.remove("good");
      b.classList.add("bad");
    }
  }

  function renderStatus() {
    const st = S.state;

    setText("stUser", S.username || "-");

    const peer = S.peer;
    setText("stPeer", peer ? (peer.username + " (" + peer.owner + ")") : "-");
    setText("stPeerOnline", peer ? (peer.online ? ("yes (seen " + peer.last_seen + ")") :
      (peer.last_seen ? ("no (last " + peer.last_seen + ")") : "no")) : "-");
    const peerPhase = (st && peer) ? ((st.activePlayer === peer.owner) ? st.phase : "waiting") : "-";
    setText("stPeerPhase", peerPhase);

    setText("stGameId", st ? String(st.gameId) : "-");
    setText("stScenario", st ? (st.scenario || "(none)") : "-");
    setText("stRound", st ? String(st.round) : "-");
    setText("stPlayer", st ? st.activePlayer : "-");
    setText("stPhase", st ? st.phase : "-");
    setText("stVP", st ? ("A:" + st.vp.A + "  B:" + st.vp.B) : "-");

    const selfOwner = (S.self && S.self.owner) ? S.self.owner : "A";
    const selfBp = st ? (selfOwner === "A" ? st.bp.A : st.bp.B) : "-";
    setText("stBP", st ? (selfOwner + ":" + selfBp) : "-");

    const note = st ? (st.notes || "") : "";
    const peerSummary = peer ? ("Peer: " + peer.username + " (" + peer.owner + ") " +
      (peer.online ? ("ONLINE (seen " + peer.last_seen + ")") :
        (peer.last_seen ? ("offline (last " + peer.last_seen + ")") : "offline"))) : "";
    setText("stNotes", note + (peerSummary ? (" | " + peerSummary) : ""));

    const stCmb = $("statusCombat");
    const elSensor = $("sensorStatus");

    // 1. Status Panel Combat Row
    if (st && st.combat && st.combat.count > 0) {
      if (stCmb) {
        stCmb.style.display = "grid";
        setText("stCmbCount", st.combat.count + " Active");
        const hexes = (st.combat.active_hexes || []).join(", ");
        setText("stCmbHexes", hexes);
      }
    } else {
      if (stCmb) stCmb.style.display = "none";
    }

    // 2. Sensor Strip
    if (elSensor) {
      let text = "✓ SENSORS CLEAR";
      let color = "var(--good)";
      let bg = "rgba(110, 231, 183, 0.15)";

      // Combat Priority
      if (st && st.combat && st.combat.count > 0) {
        text = "⚠️ COMBAT ACTIVE";
        color = "var(--bad)";
        bg = "rgba(251, 113, 133, 0.15)";
      } else {
        // Enemy Check
        const selfOwner = (S.self && S.self.owner) ? S.self.owner : "?";
        // Basic check: if 'ships' array exists in state and contains other owner
        if (st.ships && Array.isArray(st.ships)) {
          for (let i = 0; i < st.ships.length; i++) {
            if (st.ships[i].owner !== selfOwner) {
              text = "⚠️ ENEMY DETECTED";
              color = "#facc15";
              bg = "rgba(250, 204, 21, 0.15)";
              break;
            }
          }
        }
      }
      elSensor.innerText = text;
      elSensor.style.color = color;
      elSensor.style.backgroundColor = bg;
    }

    setLoginBadge();
  }

  async function apiFetchState() {
    const j = await apiJson("state", "GET", null, true);
    S.state = j.state;
    S.self = j.self || null;
    S.peer = j.peer || null;
    renderStatus();

    // Poll Combat Log (Feedback Loop)
    if (j.state && j.state.combat && j.state.combat.combats) {
      S.lastCombatLogs = S.lastCombatLogs || {};
      const list = j.state.combat.combats;
      for (let i = 0; i < list.length; i++) {
        const c = list[i];
        if (c.log && c.log !== S.lastCombatLogs[c.hex]) {
          if (S.lastCombatLogs[c.hex] && c.log.length > 0) {
            appendLine("--- Combat Update (" + c.hex + ") ---", "line-bad");
            appendLine(c.log);
          }
          S.lastCombatLogs[c.hex] = c.log;
        }
      }
    }

    return j;
  }

  async function apiLogin(username, password) {
    const j = await apiJson("login", "POST", { username: username, password: password }, false);
    S.username = username;
    S.token = j.token;
    setLoginBadge();
    appendLine("Login OK.", "line-good");
    if (j.git_sha) appendLine("SHA: " + j.git_sha, "line-muted");
    await apiFetchState();
    return j;
  }

  async function apiLogout() {
    try { await apiJson("logout", "POST", {}, true); } catch (e) { /* ignore */ }
    S.token = null;
    S.username = null;
    S.self = null;
    S.peer = null;
    setLoginBadge();
    appendLine("Logged out.", "line-muted");
    renderStatus();
  }

  async function apiCommand(cmd) {
    const j = await apiJson("command", "POST", { command: cmd }, true);

    if (j && typeof j.event === "string" && j.event.length > 0) {
      const parts = j.event.split("\n");
      for (let i = 0; i < parts.length; i++) {
        const line = parts[i];
        if (line.length) {
          appendLine(line);
        }
      }
    }

    await apiFetchState();
    return j;
  }

  function toggleMapView() {
    const mv = $("mapView");
    const log = $("consoleLog");
    const btn = $("btnMap");
    if (!mv || !log) return;

    const showingMap = (window.getComputedStyle(mv).display !== "none");
    if (showingMap) {
      mv.style.display = "none";
      log.style.display = "block";
      if (btn) btn.textContent = "Map";
    } else {
      log.style.display = "none";
      mv.style.display = "block";
      if (btn) btn.textContent = "Log";
    }
  }

  // Export
  window.KEPLERHORIZON.behavior = {
    appendLine: appendLine,
    apiLogin: apiLogin,
    apiLogout: apiLogout,
    apiFetchState: apiFetchState,
    apiCommand: apiCommand,
    toggleMapView: toggleMapView
  };

})();
