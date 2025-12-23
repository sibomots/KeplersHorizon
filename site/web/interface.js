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

  function promptLogin() {
    const u = prompt("Username:", "alice");
    if (u === null) return;
    const p = prompt("Password:", "alicepw");
    if (p === null) return;
    B.apiLogin(u, p)
      .then(() => B.apiFetchState())
      .catch(e => B.appendLine(`Login error: ${e.message}`, "line-bad"));
  }

  function wire() {
   // Periodic state polling so peer 
   // presence / phase updates show without user commands.

   if (!window.__khPollTimer) {
      window.__khPollTimer = setInterval(async () => {
          try {
            const B = window.KEPLERHORIZON
                     && window.KEPLERHORIZON.behavior
                      ? window.KEPLERHORIZON.behavior : null;
            const S = window.KEPLERHORIZON
                     && window.KEPLERHORIZON.slate
                      ? window.KEPLERHORIZON.slate : null;
            if (B && S && S.token) {
            await B.apiFetchState();
            }
            } catch (e) {
                 // ignore transient polling errors
            }
      }, 3000);
   }


    const inp = document.getElementById("commandInput");
    const send = document.getElementById("btnSend");
    const btnLogin = document.getElementById("btnLogin");
    const btnLogout = document.getElementById("btnLogout");
    const btnMap = document.getElementById("btnMap");

    btnLogin.addEventListener("click", promptLogin);
    btnLogout.addEventListener("click", () => B.apiLogout().catch(() => {}));
    btnMap.addEventListener("click", () => B.toggleMapView());

    async function runCmd() {
      const cmd = inp.value.trim();
      if (!cmd) {
         return;
      }

      B.appendLine(`> ${cmd}`, "");
      inp.value = "";
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
    B.appendLine("Login to begin. Demo: alice/alicepw, bob/bobpw.", "line-muted");
  }

  window.addEventListener("DOMContentLoaded", wire);
})();
