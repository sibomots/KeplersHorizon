// Room (waiting room) JavaScript - handles room state, start game
(function () {
    'use strict';

    const API_BASE = '/kh/api';
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

        // Seat B
        const seatBEl = document.getElementById('seatB');
        const seatBName = document.getElementById('seatBName');
        if (room.seat_b) {
            seatBName.textContent = room.seat_b_name;
            seatBName.classList.remove('waiting');
            seatBEl.classList.remove('empty');
            seatBEl.classList.add('filled');
            if (room.seat_b_name === getUsername()) {
                seatBEl.classList.add('you');
                currentUserId = room.seat_b;
            }
        } else {
            seatBName.textContent = 'Waiting...';
            seatBName.classList.add('waiting');
            seatBEl.classList.add('empty');
            seatBEl.classList.remove('filled', 'you');
        }

        // Scenario - use radio buttons
        if (room.scenario) {
            const radio = document.querySelector(`input[name="scenario"][value="${room.scenario}"]`);
            if (radio) radio.checked = true;
        }

        // Status and start button
        const statusEl = document.getElementById('statusText');
        const startBtn = document.getElementById('btnStart');

        if (room.status === 'playing') {
            // Game already started, redirect to game
            window.location.href = 'game.html';
            return;
        }

        if (room.status === 'ready') {
            statusEl.innerHTML = '<span class="status-ready">✓ Both players ready!</span>';
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

    async function setScenario() {
        const selected = document.querySelector('input[name="scenario"]:checked');
        const scenario = selected ? selected.value : 'basic';
        await apiCall('/rooms/' + roomCode + '/scenario', 'POST', { scenario });
    }

    async function startGame() {
        const selected = document.querySelector('input[name="scenario"]:checked');
        const scenario = selected ? selected.value : 'basic';
        const data = await apiCall('/rooms/' + roomCode + '/start', 'POST', { scenario });

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
        // Scenario radio buttons
        document.querySelectorAll('input[name="scenario"]').forEach(radio => {
            radio.addEventListener('change', setScenario);
        });
        document.getElementById('btnStart').addEventListener('click', startGame);
        document.getElementById('btnLeave').addEventListener('click', leaveRoom);

        // Initial load
        loadRoom();

        // Poll for updates
        pollTimer = setInterval(loadRoom, 2000);
    }

    init();
})();
