-- Migration: Add AI Agent User
-- Purpose: Create dedicated user account for AI player in single-player games
-- Date: 2026-01-27

USE khdb;

-- Add AI user with id=3
-- password_plain is NULL because AI doesn't authenticate via login
INSERT IGNORE INTO users(id, username, password_plain)
VALUES(3, 'AI_AGENT', NULL);

-- Verify insertion
SELECT id, username, created_at FROM users WHERE username = 'AI_AGENT';

-- Note: AI user will be assigned to seat_b when single-player game is created
-- The singleplayer flag is stored in rooms table
