// Lobby page JavaScript - handles room list, create, join
(function () {
    'use strict';

    const API_BASE = '/kh/api';
    let pollTimer = null;

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

    function renderRoomList(rooms, onlineCount) {
        document.getElementById('onlineCount').textContent = onlineCount || 0;

        const container = document.getElementById('roomList');

        if (!rooms || rooms.length === 0) {
            container.innerHTML = `
        <div class="empty-state">
          <p>No open rooms</p>
          <p class="small">Create one or join by code</p>
        </div>
      `;
            return;
        }

        let html = `
      <table class="table table-hover">
        <thead>
          <tr>
            <th>Name</th>
            <th>Players</th>
            <th>Status</th>
            <th></th>
          </tr>
        </thead>
        <tbody>
    `;

        for (const room of rooms) {
            const seatA = room.seat_a_name || '--';
            const seatB = room.seat_b_name || '--';
            const canJoin = !room.is_full && room.status === 'waiting';

            html += `
        <tr>
          <td>${escapeHtml(room.name || 'Unnamed')}</td>
          <td class="seats-display">
            <span class="${room.seat_a ? 'filled' : 'empty'}">${escapeHtml(seatA)}</span> vs 
            <span class="${room.seat_b ? 'filled' : 'empty'}">${escapeHtml(seatB)}</span>
          </td>
          <td><span class="badge bg-${room.status === 'ready' ? 'success' : 'secondary'}">${room.status}</span></td>
          <td>
            ${canJoin ? `<button class="btn btn-sm btn-primary btn-join" data-code="${room.room_code}">Join</button>` : ''}
          </td>
        </tr>
      `;
        }

        html += '</tbody></table>';
        container.innerHTML = html;

        // Attach join handlers
        container.querySelectorAll('.btn-join').forEach(btn => {
            btn.addEventListener('click', () => joinRoom(btn.dataset.code));
        });
    }

    async function loadRooms() {
        const data = await apiCall('/rooms');
        if (data && data.ok) {
            renderRoomList(data.rooms, data.online_count);
        }
    }

    // loadSaves() and loadSavedGame() removed - use 'save', 'load' console commands in-game

    async function createRoom() {
        const name = document.getElementById('roomName').value.trim() || 'Game Room';
        const data = await apiCall('/rooms', 'POST', { name });

        if (data && data.ok) {
            bootstrap.Modal.getInstance(document.getElementById('createModal')).hide();
            window.location.href = 'room.html?code=' + data.room.room_code;
        } else {
            alert(data?.error || 'Failed to create room');
        }
    }

    async function joinRoom(code) {
        const data = await apiCall('/rooms/' + code + '/join', 'POST');

        if (data && data.ok) {
            window.location.href = 'room.html?code=' + code;
        } else {
            alert(data?.error || 'Failed to join room');
        }
    }

    async function joinByCode() {
        const code = document.getElementById('joinCode').value.trim().toUpperCase();
        const errorEl = document.getElementById('joinError');

        if (!code || code.length !== 6) {
            errorEl.textContent = 'Enter a 6-character room code';
            errorEl.classList.remove('d-none');
            return;
        }

        errorEl.classList.add('d-none');
        const data = await apiCall('/rooms/' + code + '/join', 'POST');

        if (data && data.ok) {
            window.location.href = 'room.html?code=' + code;
        } else {
            errorEl.textContent = data?.error || 'Room not found or full';
            errorEl.classList.remove('d-none');
        }
    }

    async function logout() {
        await apiCall('/logout', 'POST');
        localStorage.removeItem('kh_token');
        localStorage.removeItem('kh_username');
        window.location.href = 'index.html';
    }

    function escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    function init() {
        if (!checkAuth()) return;

        document.getElementById('username').textContent = getUsername();

        document.getElementById('btnLogout').addEventListener('click', logout);
        document.getElementById('btnCreateRoom').addEventListener('click', createRoom);
        document.getElementById('btnJoinCode').addEventListener('click', joinByCode);
        document.getElementById('joinCode').addEventListener('keydown', (e) => {
            if (e.key === 'Enter') joinByCode();
        });

        // Initial load
        loadRooms();
        // loadSaves() removed - use console commands in-game

        // Poll for updates
        pollTimer = setInterval(loadRooms, 3000);
    }

    init();
})();
