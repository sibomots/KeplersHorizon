///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
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

  // saveGame() removed - use 'save <name>' console command instead

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
    const btnLobby = document.getElementById("btnLobby");
    const btnMap = document.getElementById("btnMap");

    if (btnLobby) btnLobby.addEventListener("click", exitToLobby);
    if (btnMap) btnMap.addEventListener("click", () => B.toggleMapView());
    // btnSave removed - use 'save <name>' console command instead

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
        B.appendLine(`Error: ${e.message}`, "line-warn");
      }
    }

    inp.addEventListener("keydown", (e) => {
      if (e.key === "Enter") runCmd();
    });

    B.appendLine("Console operational", "");
    B.appendLine("Type 'help' for information.", "");
    B.appendLine("Main Phases of Turn:", "");
    B.appendLine("1. Victory Point Check", "");
    B.appendLine("2. Build Ships", "");
    B.appendLine("3. Movement", "");
    B.appendLine("4. Combat (*)", "");
    B.appendLine("5. Repair/Resupply", "");
    B.appendLine("Command 'Next' (N) will evolve turn phase.","");
    B.appendLine("Command 'Done' will end your turn.", "");
    // Fetch initial state
    B.apiFetchState().catch(() => { });
  }

  window.addEventListener("DOMContentLoaded", wire);
})();


// Add to interface.js or behavior.js
// Enable console scrolling with Page Up/Down and Arrow keys

(function() {
  const consoleLog = document.getElementById('consoleLog');
  
  // Keyboard scrolling
  document.addEventListener('keydown', (e) => {
    if (!consoleLog) return;
    
    switch(e.key) {
      case 'PageUp':
        e.preventDefault();
        consoleLog.scrollTop -= consoleLog.clientHeight * 0.8;
        break;
      case 'PageDown':
        e.preventDefault();
        consoleLog.scrollTop += consoleLog.clientHeight * 0.8;
        break;
      case 'Home':
        if (e.ctrlKey) {
          e.preventDefault();
          consoleLog.scrollTop = 0;
        }
        break;
      case 'End':
        if (e.ctrlKey) {
          e.preventDefault();
          consoleLog.scrollTop = consoleLog.scrollHeight;
        }
        break;
    }
  });
  
  // Mouse wheel scrolling on the console area (left side)
  // Create invisible scrollable overlay on left 50%
  const scrollOverlay = document.createElement('div');
  scrollOverlay.style.cssText = `
    position: absolute;
    left: 0;
    top: 0;
    width: 50%;
    height: calc(100% - 70px);
    z-index: 3;
    pointer-events: auto;
  `;
  
  scrollOverlay.addEventListener('wheel', (e) => {
    e.preventDefault();
    consoleLog.scrollTop += e.deltaY;
  });
  
  const viewport = document.querySelector('.viewport');
  if (viewport) {
    viewport.appendChild(scrollOverlay);
  }
})();
