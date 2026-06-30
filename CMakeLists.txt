cmake_minimum_required(VERSION 3.10)
project(axioma-spec C)

# DVEC: v1.3 / DETERMINISM: D1 — Strict Deterministic
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Spey Systems LTD. All rights reserved.
# Patent: UK GB2521625.0

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror -Wpedantic)

include_directories(include)

add_library(axilog STATIC src/axilog_sha256.c)
