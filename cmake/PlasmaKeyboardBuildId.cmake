# SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
# SPDX-License-Identifier: BSD-2-Clause
#
# Runs at build time (cmake -P) and writes the git commit of the source tree to a header,
# so every build can say which commit it came from. The header is only rewritten when the
# commit changes, so unchanged builds don't recompile anything.
#
#   -DSOURCE_DIR=<source tree> -DOUTPUT=<header>
#
# Copies of the tree without .git can hold "<commit> <YYYY-MM-DD>" in .build-commit instead.

set(commit "unknown")
set(commit_date "")

find_program(GIT_EXECUTABLE git)
if(GIT_EXECUTABLE AND EXISTS "${SOURCE_DIR}/.git")
    # safe.directory: the build may run as a different user than the one owning the checkout
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -c safe.directory=* -C "${SOURCE_DIR}" describe --always --abbrev=7 --dirty=+ --exclude=*
        OUTPUT_VARIABLE git_commit
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE git_result
        ERROR_QUIET
    )
    if(git_result EQUAL 0 AND git_commit)
        set(commit "${git_commit}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -c safe.directory=* -C "${SOURCE_DIR}" log -1 --format=%cs
            OUTPUT_VARIABLE commit_date
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
    endif()
elseif(EXISTS "${SOURCE_DIR}/.build-commit")
    file(READ "${SOURCE_DIR}/.build-commit" build_commit)
    string(STRIP "${build_commit}" build_commit)
    string(REPLACE " " ";" build_commit "${build_commit}")
    list(LENGTH build_commit build_commit_length)
    if(build_commit_length GREATER 0)
        list(GET build_commit 0 commit)
    endif()
    if(build_commit_length GREATER 1)
        list(GET build_commit 1 commit_date)
    endif()
endif()

set(content "#pragma once\n#define PLASMA_KEYBOARD_BUILD_COMMIT \"${commit}\"\n#define PLASMA_KEYBOARD_BUILD_COMMIT_DATE \"${commit_date}\"\n")

if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" previous)
endif()
if(NOT "${previous}" STREQUAL "${content}")
    file(WRITE "${OUTPUT}" "${content}")
endif()
