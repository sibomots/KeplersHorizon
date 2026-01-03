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
    if (S && S.token && S.username) {
      b.textContent = S.username;
      b.classList.remove("bad");
      b.classList.add("good");
    } else {
      b.textContent = "not logged in";
      b.classList.remove("good");
      b.classList.add("bad");
    }
  }

  function renderStatus() {
    const st = S.state;

    // Peer info (kept for internal use, no longer displayed)
    const peer = S.peer;
    const peerPhase = (st && peer) ? ((st.activePlayer === peer.owner) ? st.phase : "waiting") : "-";

    // Basic state fields (GameId, Round, etc. still displayed)
    setText("stGameId", st ? String(st.gameId) : "-");
    setText("stRound", st ? String(st.round) : "-");
    setText("stPlayer", st ? st.activePlayer : "-");
    setText("stPhase", st ? st.phase : "-");
    setText("stVP", st ? ("A:" + st.vp.A + "  B:" + st.vp.B) : "-");

    const selfOwner = (S.self && S.self.owner) ? S.self.owner : "A";
    const selfBp = st ? (selfOwner === "A" ? st.bp.A : st.bp.B) : "-";
    setText("stBP", st ? (selfOwner + ":" + selfBp) : "-");

    // Bug 5: Update console title with player and scenario
    const elTitle = $("consoleTitle");
    if (elTitle) {
      const scenario = (st && st.scenario) ? st.scenario : "";
      const playerName = S.username || "Player";
      if (scenario) {
        elTitle.textContent = "Player " + playerName + " Main Console - Scenario: " + scenario;
      } else {
        elTitle.textContent = "Player " + playerName + " Main Console";
      }
    }

    // Bug 3: Turn status indicator (green = your turn, yellow = opponent's turn)
    const elTurn = $("turnStatus");
    if (elTurn) {
      const isYourTurn = st && S.self && (st.activePlayer === S.self.owner);
      if (st && st.scenario) {
        if (isYourTurn) {
          elTurn.textContent = "Your Turn";
          elTurn.style.backgroundColor = "rgba(110, 231, 183, 0.25)";
          elTurn.style.color = "var(--good)";
        } else {
          const oppName = (peer && peer.username) ? peer.username : "Opponent";
          elTurn.textContent = oppName + "'s Turn";
          elTurn.style.backgroundColor = "rgba(250, 204, 21, 0.25)";
          elTurn.style.color = "#facc15";
        }
      } else {
        elTurn.textContent = "--";
        elTurn.style.backgroundColor = "transparent";
        elTurn.style.color = "var(--muted)";
      }
    }

    // Bug 4: Peer online status indicator
    const elPeerOnline = $("peerOnlineStatus");
    if (elPeerOnline) {
      if (peer && peer.online) {
        elPeerOnline.textContent = peer.username + " is online";
        elPeerOnline.style.backgroundColor = "rgba(110, 231, 183, 0.25)";
        elPeerOnline.style.color = "var(--good)";
      } else {
        elPeerOnline.textContent = "Waiting for opponent";
        elPeerOnline.style.backgroundColor = "rgba(250, 204, 21, 0.15)";
        elPeerOnline.style.color = "#facc15";
      }
    }

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
      let text = "SENSORS CLEAR";
      let color = "var(--good)";
      let bg = "rgba(110, 231, 183, 0.15)";

      // Check if player has any ships deployed
      const checkOwner = (S.self && S.self.owner) ? S.self.owner : "?";
      let playerHasShips = false;
      let enemyColocated = false;

      if (st && st.ships && Array.isArray(st.ships)) {
        for (let i = 0; i < st.ships.length; i++) {
          if (st.ships[i].owner === checkOwner) {
            playerHasShips = true;
          } else {
            // Check if enemy is in same hex as any of our ships
            for (let j = 0; j < st.ships.length; j++) {
              if (st.ships[j].owner === checkOwner &&
                st.ships[j].at_hex === st.ships[i].at_hex) {
                enemyColocated = true;
                break;
              }
            }
          }
        }
      }

      // Priority: Combat > No Ships > Enemy Colocated > Clear
      if (st && st.combat && st.combat.count > 0) {
        text = "COMBAT ACTIVE";
        color = "var(--bad)";
        bg = "rgba(251, 113, 133, 0.15)";
      } else if (!playerHasShips) {
        text = "SENSORS OFFLINE";
        color = "var(--muted)";
        bg = "rgba(168, 168, 194, 0.15)";
      } else if (enemyColocated) {
        text = "ENEMY DETECTED";
        color = "#facc15";
        bg = "rgba(250, 204, 21, 0.15)";
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

    // Display queued messages (tell/broadcast notifications)
    if (j.messages && Array.isArray(j.messages)) {
      for (let i = 0; i < j.messages.length; i++) {
        // Handle escaped newlines in messages
        const msgLines = j.messages[i].replace(/\\n/g, "\n").split("\n");
        for (let k = 0; k < msgLines.length; k++) {
          if (msgLines[k].length > 0) {
            appendLine((k === 0 ? ">> " : "   ") + msgLines[k], "line-bad");
          }
        }
      }
    }

    // Poll Combat Log (Feedback Loop)
    if (j.state && j.state.combat && j.state.combat.combats) {
      S.lastCombatLogs = S.lastCombatLogs || {};
      const list = j.state.combat.combats;
      for (let i = 0; i < list.length; i++) {
        const c = list[i];
        if (c.log && c.log !== S.lastCombatLogs[c.hex]) {
          if (S.lastCombatLogs[c.hex] && c.log.length > 0) {
            appendLine("--- Combat Update (" + c.hex + ") ---", "line-bad");
            // Handle escaped newlines in combat log
            const logLines = c.log.replace(/\\n/g, "\n").split("\n");
            for (let k = 0; k < logLines.length; k++) {
              if (logLines[k].trim().length > 0) {
                appendLine(logLines[k]);
              }
            }
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
      if (btn) btn.textContent = "Show Map";
    } else {
      log.style.display = "none";
      mv.style.display = "block";
      if (btn) btn.textContent = "Show Log";
    }
  }

  function handleHeartbeatCommand(cmd) {
    const parts = cmd.trim().toLowerCase().split(/\s+/);
    if (parts[0] !== "hb") return false;

    const mode = parts[1] || "";
    if (mode === "normal") {
      S.hbMode = "normal";
      appendLine("Heartbeat: normal (3s interval)", "line-good");
      return true;
    } else if (mode === "off") {
      S.hbMode = "off";
      appendLine("Heartbeat: off", "line-muted");
      return true;
    } else if (mode === "shot") {
      appendLine("Heartbeat: one-shot", "line-muted");
      apiFetchState().catch(() => { });
      return true;
    }
    appendLine("Usage: hb normal | hb off | hb shot", "line-bad");
    return true;
  }

  // Export
  window.KEPLERHORIZON.behavior = {
    appendLine: appendLine,
    apiLogin: apiLogin,
    apiLogout: apiLogout,
    apiFetchState: apiFetchState,
    apiCommand: apiCommand,
    toggleMapView: toggleMapView,
    handleHeartbeatCommand: handleHeartbeatCommand
  };

})();
