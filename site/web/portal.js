// Portal page JavaScript - handles login only
(function () {
    'use strict';

    const API_BASE = '/kh/api';

    function showAlert(message, type) {
        const box = document.getElementById('alertBox');
        box.className = `alert alert-${type}`;
        box.textContent = message;
        box.classList.remove('d-none');
    }

    function hideAlert() {
        document.getElementById('alertBox').classList.add('d-none');
    }

    async function apiCall(endpoint, body) {
        const resp = await fetch(API_BASE + endpoint, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        });
        return resp.json();
    }

    // Check if already logged in
    function checkSession() {
        const token = localStorage.getItem('kh_token');
        if (token) {
            // Verify token is still valid
            fetch(API_BASE + '/state', {
                headers: { 'Authorization': 'Bearer ' + token }
            }).then(r => r.json()).then(data => {
                if (data.ok) {
                    // Already logged in, go to lobby
                    window.location.href = 'lobby.html';
                } else {
                    // Token invalid, clear it
                    localStorage.removeItem('kh_token');
                    localStorage.removeItem('kh_username');
                }
            }).catch(() => {
                // Server not reachable, stay on portal
            });
        }
    }

    // Login form handler
    document.getElementById('loginForm').addEventListener('submit', async function (e) {
        e.preventDefault();
        hideAlert();

        const username = document.getElementById('loginUsername').value.trim();
        const password = document.getElementById('loginPassword').value;

        try {
            const data = await apiCall('/login', { username, password });

            if (data.ok) {
                localStorage.setItem('kh_token', data.token);
                localStorage.setItem('kh_username', data.username);
                showAlert('Login successful! Redirecting...', 'success');
                setTimeout(() => window.location.href = 'lobby.html', 500);
            } else {
                showAlert(data.error || 'Login failed', 'danger');
            }
        } catch (err) {
            showAlert('Connection error: ' + err.message, 'danger');
        }
    });

    // Bridge settings check - minimum window dimensions
    document.getElementById('btnBridgeCheck').addEventListener('click', function () {
        const minW = 1200, minH = 800;
        const w = window.innerWidth;
        const h = window.innerHeight;
        const status = document.getElementById('bridgeStatus');

        if (w >= minW && h >= minH) {
            status.style.color = 'var(--good)';
            status.innerHTML = '✓ BRIDGE SYSTEMS NOMINAL<br>' +
                'Viewport: ' + w + ' × ' + h + ' px';
        } else {
            status.style.color = 'var(--bad)';
            status.innerHTML = '⚠ VIEWPORT INSUFFICIENT<br>' +
                'Current: ' + w + ' × ' + h + ' px<br>' +
                'Required: ' + minW + ' × ' + minH + ' px minimum';
        }
    });

    // Check session on load
    checkSession();
})();
