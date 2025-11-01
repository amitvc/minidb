//
// Created by Amit Chavan on 8/30/25.
//

#pragma once

#include <cstdint>

/**
 * @file config.h
 * @brief Contains compile-time constants and type definitions for the database.
 */

// Use a type alias for page IDs for clarity and future flexibility.
using page_id_t = int32_t;

/**
 * @brief The size of a single page in bytes.
 *
 * All disk I/O will be in chunk size of 4096 bytes.
 */
static constexpr int PAGE_SIZE = 4096; // 4kb pages

static constexpr int EXTENT_SIZE = 8;       // 8 pages per extent
static constexpr int INVALID_PAGE_ID = -1;
static constexpr int HEADER_PAGE_ID = 0;
static constexpr page_id_t FIRST_GAM_PAGE_ID = 1;



