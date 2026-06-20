#!/usr/bin/env bash
# boostApp build script
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
INSTALL_PREFIX="${INSTALL_PREFIX:-${ROOT_DIR}/out}"
THIRD_PARTY_LIB_DIR="${THIRD_PARTY_LIB_DIR:-${ROOT_DIR}/lib}"

BOOST_VERSION="${BOOST_VERSION:-1.84.0}"
BOOST_VERSION_TAG="${BOOST_VERSION//./_}"
BOOST_DEPS_DIR="${ROOT_DIR}/deps"
BOOST_SRC_DIR="${BOOST_DEPS_DIR}/boost_${BOOST_VERSION_TAG}"
BOOST_INSTALL_DIR="${THIRD_PARTY_LIB_DIR}/boost"
BOOST_LEGACY_INSTALL_DIR="${ROOT_DIR}/deps/boost/install"
BOOST_LEGACY_SRC_DIR="${ROOT_DIR}/deps/boost/src/boost_${BOOST_VERSION_TAG}"
BOOST_TARBALL="${BOOST_DEPS_DIR}/boost_${BOOST_VERSION_TAG}.tar.bz2"
BOOST_URLS=(
    "https://archives.boost.io/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_TAG}.tar.bz2"
    "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_TAG}.tar.bz2"
)

usage() {
    cat <<EOF
Usage: $(basename "$0") [command] [options]

Commands:
  build         Build Boost (if needed), configure and compile (default)
  rebuild       Clean build directory and compile
  boost         Download and compile Boost from source only
  clean         Remove build directory
  clean-boost   Remove compiled Boost from lib/boost/
  clean-lib     Remove all third-party libraries under lib/
  clean-all     Remove build directory and lib/
  install       Build and install boostApp to prefix
  run           Build and run a quick pub/sub smoke test

Options:
  -t, --type TYPE     Build type: Release or Debug (default: Release)
  -j, --jobs N        Parallel jobs (default: ${JOBS})
  -d, --dir PATH      Build directory (default: build/)
  -p, --prefix PATH   boostApp install prefix (default: out/)
  -h, --help          Show this help

Environment:
  BUILD_TYPE, BUILD_DIR, JOBS, INSTALL_PREFIX, BOOST_VERSION, THIRD_PARTY_LIB_DIR

Third-party libraries (compiled):
  lib/boost/         Boost install prefix (include/, lib/, etc.)

Boost sources:
  Source tarball : ${BOOST_TARBALL}
  Source tree    : ${BOOST_SRC_DIR}

Examples:
  $(basename "$0") boost              # first-time: compile Boost from source
  $(basename "$0")                    # build boostApp
  $(basename "$0") rebuild -t Debug
  BOOST_VERSION=1.85.0 $(basename "$0") boost
EOF
}

log() {
    printf '[build] %s\n' "$*"
}

die() {
    printf '[build] error: %s\n' "$*" >&2
    exit 1
}

check_toolchain_deps() {
    command -v cmake >/dev/null 2>&1 || die "cmake not found"
    command -v g++ >/dev/null 2>&1 || die "g++ not found"
}

check_boost_build_deps() {
    check_toolchain_deps
    command -v tar >/dev/null 2>&1 || die "tar not found"
    if ! command -v curl >/dev/null 2>&1 && ! command -v wget >/dev/null 2>&1; then
        die "curl or wget required to download Boost"
    fi
}

download_boost() {
    if [[ -f "${BOOST_TARBALL}" ]]; then
        log "boost tarball exists: ${BOOST_TARBALL}"
        return 0
    fi

    mkdir -p "${BOOST_DEPS_DIR}"
    log "downloading Boost ${BOOST_VERSION} (source dir ${BOOST_SRC_DIR} not found)..."

    local url
    for url in "${BOOST_URLS[@]}"; do
        log "try: ${url}"
        if command -v curl >/dev/null 2>&1; then
            if curl -fsSL -o "${BOOST_TARBALL}" "${url}"; then
                log "downloaded from ${url}"
                return 0
            fi
        elif wget -O "${BOOST_TARBALL}" "${url}"; then
            log "downloaded from ${url}"
            return 0
        fi
        rm -f "${BOOST_TARBALL}"
    done

    die "failed to download Boost ${BOOST_VERSION}; check network or set BOOST_VERSION"
}

extract_boost() {
    if [[ -d "${BOOST_SRC_DIR}" ]]; then
        log "boost source exists: ${BOOST_SRC_DIR}"
        return 0
    fi

    if [[ ! -f "${BOOST_TARBALL}" ]]; then
        die "Boost source missing at ${BOOST_SRC_DIR} and no tarball at ${BOOST_TARBALL}"
    fi

    log "extracting ${BOOST_TARBALL} -> ${BOOST_DEPS_DIR}/"
    tar -xjf "${BOOST_TARBALL}" -C "${BOOST_DEPS_DIR}"
}

ensure_boost_source() {
    if [[ -d "${BOOST_SRC_DIR}" && -f "${BOOST_SRC_DIR}/bootstrap.sh" ]]; then
        log "using local Boost source: ${BOOST_SRC_DIR}"
        return 0
    fi

    download_boost
    extract_boost

    if [[ ! -d "${BOOST_SRC_DIR}" || ! -f "${BOOST_SRC_DIR}/bootstrap.sh" ]]; then
        die "Boost source not found at ${BOOST_SRC_DIR}"
    fi
}

build_boost_from_source() {
    check_boost_build_deps
    ensure_boost_source

    mkdir -p "${THIRD_PARTY_LIB_DIR}"
    log "compiling Boost ${BOOST_VERSION} -> ${BOOST_INSTALL_DIR}"
    log "this may take several minutes..."

    (
        cd "${BOOST_SRC_DIR}"
        ./bootstrap.sh --prefix="${BOOST_INSTALL_DIR}"
        ./b2 -j"${JOBS}" \
            variant=release \
            link=shared \
            threading=multi \
            --without-mpi \
            --without-python \
            --without-test \
            --without-graph \
            --without-graph_parallel \
            install
    )

    [[ -f "${BOOST_INSTALL_DIR}/include/boost/version.hpp" ]] \
        || die "Boost install failed: ${BOOST_INSTALL_DIR}/include/boost/version.hpp missing"

    log "boost installed: ${BOOST_INSTALL_DIR}"
}

ensure_boost() {
    if [[ -d "${BOOST_LEGACY_SRC_DIR}" && ! -d "${BOOST_SRC_DIR}" ]]; then
        log "migrating legacy Boost source -> ${BOOST_SRC_DIR}"
        mv "${BOOST_LEGACY_SRC_DIR}" "${BOOST_SRC_DIR}"
    fi

    if [[ -f "${BOOST_LEGACY_INSTALL_DIR}/include/boost/version.hpp" \
          && ! -f "${BOOST_INSTALL_DIR}/include/boost/version.hpp" ]]; then
        log "migrating legacy Boost install -> ${BOOST_INSTALL_DIR}"
        mkdir -p "${THIRD_PARTY_LIB_DIR}"
        mv "${BOOST_LEGACY_INSTALL_DIR}" "${BOOST_INSTALL_DIR}"
    fi

    if [[ -f "${BOOST_INSTALL_DIR}/include/boost/version.hpp" ]]; then
        log "using local Boost: ${BOOST_INSTALL_DIR}"
        return 0
    fi
    build_boost_from_source
}

configure() {
    ensure_boost
    log "configure: type=${BUILD_TYPE}, dir=${BUILD_DIR}, Boost_ROOT=${BOOST_INSTALL_DIR}"
    cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DBOOSTAPP_BOOST_ROOT="${BOOST_INSTALL_DIR}" \
        -DBoost_NO_SYSTEM_PATHS=ON
}

compile() {
    log "compile: jobs=${JOBS}"
    cmake --build "${BUILD_DIR}" --parallel "${JOBS}"
}

do_clean() {
    log "clean build dir: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
}

do_clean_boost() {
    log "clean boost: ${BOOST_INSTALL_DIR}"
    rm -rf "${BOOST_INSTALL_DIR}"
}

do_clean_lib() {
    log "clean third-party lib dir: ${THIRD_PARTY_LIB_DIR}"
    rm -rf "${THIRD_PARTY_LIB_DIR}"
}

do_clean_all() {
    do_clean
    do_clean_lib
}

do_build() {
    check_toolchain_deps
    mkdir -p "${BUILD_DIR}"
    configure
    compile
    log "done:"
    log "  ${BUILD_DIR}/nvcommd"
    log "  ${BUILD_DIR}/nvcomm_pub"
    log "  ${BUILD_DIR}/nvcomm_sub"
}

do_rebuild() {
    do_clean
    do_build
}

do_install() {
    do_build
    log "install boostApp: prefix=${INSTALL_PREFIX}"
    cmake --install "${BUILD_DIR}" --prefix "${INSTALL_PREFIX}"
    log "installed to ${INSTALL_PREFIX}"
}

do_run() {
    do_build
    mkdir -p "${ROOT_DIR}/log" "${ROOT_DIR}/data/events"

    log "smoke test: daemon + subscriber + publisher"
    "${BUILD_DIR}/nvcommd" &
    local daemon_pid=$!
    sleep 0.5

    "${BUILD_DIR}/nvcomm_sub" sensor/temp &
    local sub_pid=$!
    sleep 0.5

    "${BUILD_DIR}/nvcomm_pub" sensor/temp 25.3
    sleep 0.5

    kill "${sub_pid}" 2>/dev/null || true
    kill "${daemon_pid}" 2>/dev/null || true
    wait "${sub_pid}" 2>/dev/null || true
    wait "${daemon_pid}" 2>/dev/null || true
    log "smoke test finished"
}

COMMAND="build"
while [[ $# -gt 0 ]]; do
    case "$1" in
        build|rebuild|boost|clean|clean-boost|clean-lib|clean-all|install|run)
            COMMAND="$1"
            shift
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
            ;;
        -d|--dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown argument: $1 (use --help)"
            ;;
    esac
done

case "${COMMAND}" in
    build) do_build ;;
    rebuild) do_rebuild ;;
    boost) build_boost_from_source ;;
    clean) do_clean ;;
    clean-boost) do_clean_boost ;;
    clean-lib) do_clean_lib ;;
    clean-all) do_clean_all ;;
    install) do_install ;;
    run) do_run ;;
    *) die "unknown command: ${COMMAND}" ;;
esac
