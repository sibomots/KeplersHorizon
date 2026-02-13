///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
// Room (waiting room) JavaScript - handles room state, start game
(function () {
    'use strict';

    const API_BASE = '/bkhZZ/api';
    let roomCode = null;
    let pollTimer = null;
    let currentUserId = null;

    function getToken() {
        return localStorage.getItem('kh_token');
    }

    function getUsername() {
        return localStorage.getItem('kh_username');
    }

    function checkAuth() {
        if (!getToken()) {
            window.location.href = 'index.html';
            return false;
        }
        return true;
    }

    async function apiCall(endpoint, method = 'GET', body = null) {
        const opts = {
            method,
            headers: {
                'Authorization': 'Bearer ' + getToken(),
                'Content-Type': 'application/json'
            }
        };
        if (body) opts.body = JSON.stringify(body);

        const resp = await fetch(API_BASE + endpoint, opts);
        const data = await resp.json();

        if (resp.status === 401) {
            localStorage.removeItem('kh_token');
            localStorage.removeItem('kh_username');
            window.location.href = 'index.html';
            return null;
        }

        return data;
    }

    function renderRoom(room) {
        document.getElementById('roomCode').textContent = room.room_code;
        document.getElementById('roomName').textContent = room.name || 'Game Room';

        // Seat A
        const seatAEl = document.getElementById('seatA');
        const seatAName = document.getElementById('seatAName');
        if (room.seat_a) {
            seatAName.textContent = room.seat_a_name;
            seatAName.classList.remove('waiting');
            seatAEl.classList.remove('empty');
            seatAEl.classList.add('filled');
            if (room.seat_a_name === getUsername()) {
                seatAEl.classList.add('you');
                currentUserId = room.seat_a;
            }
        } else {
            seatAName.textContent = 'Waiting...';
            seatAName.classList.add('waiting');
            seatAEl.classList.add('empty');
            seatAEl.classList.remove('filled', 'you');
        }

        // Seat B - NEW: Handle single-player mode
        const seatBEl = document.getElementById('seatB');
        const seatBName = document.getElementById('seatBName');
        const isSinglePlayer = document.querySelector('input[name="gameMode"]:checked')?.value === 'singleplayer';
        
        // Clear previous AI styling
        seatBEl.classList.remove('ai');
        seatBName.classList.remove('ai');
        
        if (isSinglePlayer) {
            // Single-player: Seat B is AI
            seatBName.textContent = 'AI AGENT';
            seatBName.classList.remove('waiting');
            seatBName.classList.add('ai');
            seatBEl.classList.remove('empty');
            seatBEl.classList.add('filled', 'ai');
        } else if (room.seat_b) {
            // Two-player: Human in seat B
            seatBName.textContent = room.seat_b_name;
            seatBName.classList.remove('waiting');
            seatBEl.classList.remove('empty');
            seatBEl.classList.add('filled');
            if (room.seat_b_name === getUsername()) {
                seatBEl.classList.add('you');
                currentUserId = room.seat_b;
            }
        } else {
            // Two-player: Waiting for human
            seatBName.textContent = 'Waiting...';
            seatBName.classList.add('waiting');
            seatBEl.classList.add('empty');
            seatBEl.classList.remove('filled', 'you');
        }

        // Module - use dropdown
        if (room.module_id) {
            const select = document.getElementById('moduleSelect');
            if (select) select.value = room.module_id;
        }

        // Status and start button - NEW: Single-player can start immediately
        const statusEl = document.getElementById('statusText');
        const startBtn = document.getElementById('btnStart');

        if (room.status === 'playing') {
            // Game already started, redirect to game
            window.location.href = 'game.html';
            return;
        }

        if (room.status === 'ready' || (isSinglePlayer && room.seat_a)) {
            statusEl.innerHTML = '<span class="status-ready">✓ Ready to start!</span>';
            startBtn.disabled = false;
        } else {
            statusEl.innerHTML = '<span class="status-waiting">Waiting for opponent...</span>';
            startBtn.disabled = true;
        }
    }

    async function loadRoom() {
        if (!roomCode) return;

        const data = await apiCall('/rooms/' + roomCode);

        if (!data || !data.ok) {
            alert('Room not found');
            window.location.href = 'lobby.html';
            return;
        }

        renderRoom(data.room);
    }

    async function setModule() {
        const select = document.getElementById('moduleSelect');
        const module_id = select ? parseInt(select.value) : 1;
        await apiCall('/rooms/' + roomCode + '/module', 'POST', { module_id });
    }

    async function startGame() {
        const select = document.getElementById('moduleSelect');
        const module_id = select ? parseInt(select.value) : 1;
        
        // NEW: Include game mode in request
        const gameMode = document.querySelector('input[name="gameMode"]:checked')?.value || 'twoplayer';
        const singleplayer = (gameMode === 'singleplayer');
        
        const data = await apiCall('/rooms/' + roomCode + '/start', 'POST', { 
            module_id,
            singleplayer  // NEW field
        });

        if (data && data.ok) {
            // Store game info for the game page
            localStorage.setItem('kh_game_id', data.game_id);
            localStorage.setItem('kh_room_code', roomCode);
            window.location.href = 'game.html';
        } else {
            alert(data?.error || 'Failed to start game');
        }
    }

    async function leaveRoom() {
        await apiCall('/rooms/' + roomCode + '/leave', 'POST');
        window.location.href = 'lobby.html';
    }

    function copyRoomCode() {
        navigator.clipboard.writeText(roomCode).then(() => {
            const el = document.getElementById('roomCode');
            const original = el.textContent;
            el.textContent = 'Copied!';
            setTimeout(() => el.textContent = original, 1000);
        });
    }

    async function loadModules() {
        try {
            const data = await apiCall('/modules');
            if (data && data.modules) {
                const select = document.getElementById('moduleSelect');
                select.innerHTML = '';
                data.modules.forEach(m => {
                    const opt = document.createElement('option');
                    opt.value = m.module_id;
                    opt.textContent = m.name;
                    select.appendChild(opt);
                });
            }
        } catch (e) {
            console.warn('Could not load modules:', e);
        }
    }

    function init() {
        if (!checkAuth()) return;

        // Get room code from URL
        const params = new URLSearchParams(window.location.search);
        roomCode = params.get('code');

        if (!roomCode) {
            window.location.href = 'lobby.html';
            return;
        }

        document.getElementById('roomCode').addEventListener('click', copyRoomCode);
        document.getElementById('moduleSelect').addEventListener('change', setModule);
        document.getElementById('btnStart').addEventListener('click', startGame);
        document.getElementById('btnLeave').addEventListener('click', leaveRoom);
        
        // NEW: Listen for game mode changes
        document.querySelectorAll('input[name="gameMode"]').forEach(radio => {
            radio.addEventListener('change', () => {
                // Re-render room to update Seat B display
                loadRoom();
            });
        });

        // Load modules from API
        loadModules();

        // Initial load
        loadRoom();

        // Poll for updates
        pollTimer = setInterval(loadRoom, 2000);
    }

    init();
})();
