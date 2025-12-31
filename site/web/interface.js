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
/////////////////////////////////////////////////////////////////////////////////
// interface.js
// Functions invoked by UI event handlers.

window.KEPLERHORIZON = window.KEPLERHORIZON || {};
(function () {
  const B = window.KEPLERHORIZON.behavior;
  const S = window.KEPLERHORIZON.slate;

  function checkAuth() {
    const token = localStorage.getItem('kh_token');
    const username = localStorage.getItem('kh_username');

    if (!token || !username) {
      window.location.href = 'index.html';
      return false;
    }

    // Load token into slate for API calls
    S.token = token;
    S.username = username;
    return true;
  }

  function exitToLobby() {
    window.location.href = 'lobby.html';
  }

  function wire() {
    // Check authentication first
    if (!checkAuth()) return;

    // Update login badge with username
    const badge = document.getElementById('loginBadge');
    if (badge) badge.textContent = S.username;

    // Periodic state polling
    if (!window.__khPollTimer) {
      window.__khPollTimer = setInterval(async () => {
        try {
          if (B && S && S.token && S.hbMode === "normal") {
            await B.apiFetchState();
          }
        } catch (e) {
          // ignore transient polling errors
        }
      }, 3000);
    }

    const inp = document.getElementById("commandInput");
    const send = document.getElementById("btnSend");
    const btnLobby = document.getElementById("btnLobby");
    const btnMap = document.getElementById("btnMap");

    if (btnLobby) btnLobby.addEventListener("click", exitToLobby);
    if (btnMap) btnMap.addEventListener("click", () => B.toggleMapView());

    async function runCmd() {
      const cmd = inp.value.trim();
      if (!cmd) {
        return;
      }

      B.appendLine(`> ${cmd}`, "");
      inp.value = "";

      // Intercept heartbeat commands (client-side only)
      if (cmd.toLowerCase().startsWith("hb ") || cmd.toLowerCase() === "hb") {
        B.handleHeartbeatCommand(cmd);
        return;
      }

      try {
        await B.apiCommand(cmd);
      } catch (e) {
        B.appendLine(`Error: ${e.message}`, "line-bad");
      }
    }

    send.addEventListener("click", runCmd);
    inp.addEventListener("keydown", (e) => {
      if (e.key === "Enter") runCmd();
    });

    B.appendLine("Kepler's Horizon client loaded.", "line-muted");
    B.appendLine("Type 'help' for commands.", "line-muted");

    // Fetch initial state
    B.apiFetchState().catch(() => { });
  }

  window.addEventListener("DOMContentLoaded", wire);
})();
