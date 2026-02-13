///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
// slate.js
// Mutable global state (client-side cache only; backend is authoritative).

window.KEPLERHORIZON = window.KEPLERHORIZON || {};
window.KEPLERHORIZON.slate = {
  token: "",
  username: "",
  state: null,
  viewMode: "log", // "log" | "map"
  hbMode: "normal", // "normal" | "off"
  hbIntervalMs: 3000,
  playerNames: { A: "PLAYER 1", B: "PLAYER 2" }  // Will be updated from server
};
