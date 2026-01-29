//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __AI_DB_MUTEX_H__
#define __AI_DB_MUTEX_H__

#include <mutex>

/**
 * @brief Shared mutex for synchronizing AI operations and database access
 *
 * This mutex prevents race conditions between:
 * 1. KHS script reloading
 * 2. AI turn execution (including all database queries)
 *
 * The mutex ensures that:
 * - AI turn cannot start during KHS reload
 * - KHS reload cannot happen during AI turn
 * - Database queries from AI are atomic with respect to script changes
 */
class AIDBMutex
{
  public:
    static AIDBMutex& instance()
    {
        static AIDBMutex _instance;
        return _instance;
    }
    static std::mutex ai_mutex;

  private:
    // Private constructor, destructor, copy constructor, and assignment
    // operator to prevent external instantiation, copying, or assignment
    AIDBMutex() = default;
    ~AIDBMutex() = default;
    AIDBMutex(const AIDBMutex&) = delete;
    AIDBMutex& operator=(const AIDBMutex&) = delete;
    AIDBMutex(AIDBMutex&&) = delete;
    AIDBMutex& operator=(AIDBMutex&&) = delete;
};

#endif // __AI_DB_MUTEX_H__
