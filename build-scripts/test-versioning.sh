#!/bin/sh
# Copyright 2022 Collabora Ltd.
# SPDX-License-Identifier: Zlib

set -eu

cd `dirname $0`/..

# Needed so sed doesn't report illegal byte sequences on macOS
export LC_CTYPE=C

header=include/SDL3_shadercross/SDL_shadercross.h
ref_major=$(sed -ne 's/^#define SDL_SHADERCROSS_MAJOR_VERSION  *//p' $header)
ref_minor=$(sed -ne 's/^#define SDL_SHADERCROSS_MINOR_VERSION  *//p' $header)
ref_micro=$(sed -ne 's/^#define SDL_SHADERCROSS_MICRO_VERSION  *//p' $header)
ref_version="${ref_major}.${ref_minor}.${ref_micro}"

tests=0
failed=0

ok () {
    tests=$(( tests + 1 ))
    echo "ok - $*"
}

not_ok () {
    tests=$(( tests + 1 ))
    echo "not ok - $*"
    failed=1
}

major=$(sed -ne 's/^set(MAJOR_VERSION \([0-9]*\))$/\1/p' CMakeLists.txt)
minor=$(sed -ne 's/^set(MINOR_VERSION \([0-9]*\))$/\1/p' CMakeLists.txt)
micro=$(sed -ne 's/^set(MICRO_VERSION \([0-9]*\))$/\1/p' CMakeLists.txt)
ref_sdl_req=$(sed -ne 's/^set(SDL_REQUIRED_VERSION \([0-9.]*\))$/\1/p' CMakeLists.txt)
version="${major}.${minor}.${micro}"

if [ "$ref_version" = "$version" ]; then
    ok "CMakeLists.txt $version"
else
    not_ok "CMakeLists.txt $version disagrees with SDL_shadercross.h $ref_version"
fi

for rcfile in src/version.rc; do
    tuple=$(sed -ne 's/^ *FILEVERSION *//p' "$rcfile" | tr -d '\r')
    ref_tuple="${ref_major},${ref_minor},${ref_micro},0"

    if [ "$ref_tuple" = "$tuple" ]; then
        ok "$rcfile FILEVERSION $tuple"
    else
        not_ok "$rcfile FILEVERSION $tuple disagrees with SDL_shadercross.h $ref_tuple"
    fi

    tuple=$(sed -ne 's/^ *PRODUCTVERSION *//p' "$rcfile" | tr -d '\r')

    if [ "$ref_tuple" = "$tuple" ]; then
        ok "$rcfile PRODUCTVERSION $tuple"
    else
        not_ok "$rcfile PRODUCTVERSION $tuple disagrees with SDL_shadercross.h $ref_tuple"
    fi

    tuple=$(sed -Ene 's/^ *VALUE "FileVersion", "([0-9, ]*)\\0"\r?$/\1/p' "$rcfile" | tr -d '\r')
    ref_tuple="${ref_major}, ${ref_minor}, ${ref_micro}, 0"

    if [ "$ref_tuple" = "$tuple" ]; then
        ok "$rcfile FileVersion $tuple"
    else
        not_ok "$rcfile FileVersion $tuple disagrees with SDL_shadercross.h $ref_tuple"
    fi

    tuple=$(sed -Ene 's/^ *VALUE "ProductVersion", "([0-9, ]*)\\0"\r?$/\1/p' "$rcfile" | tr -d '\r')

    if [ "$ref_tuple" = "$tuple" ]; then
        ok "$rcfile ProductVersion $tuple"
    else
        not_ok "$rcfile ProductVersion $tuple disagrees with SDL_shadercross.h $ref_tuple"
    fi
done

echo "1..$tests"
exit "$failed"
