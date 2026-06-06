#! /bin/bash

# Common deployment functions for segar workspace

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

function clear_dirs() {
    rm -rf "$build_dir"
    rm -rf "$install_dir"
}

function build_and_install() {
    local cmake_args=(-DPLATFORM_NAME=$platform_name -DINSTALL_DIR=$install_dir)
    cmake -B $build_dir -S "$SCRIPT_DIR" "${cmake_args[@]}"
    rm -rf "$build_dir" $PROJECT_ROOT/scripts/deploy/install
}

function reorganize_deploy_install_layout() {
    # Normalize layout under install_dir: metadata under share/version, config under
    # share/config, shell helpers and msg idl under share/, drop dist/tools/scripts.
    # Idempotent: safe to call again after copying segar_config into share/.
    if [ -z "${install_dir:-}" ]; then
        echo "deploy_base: install_dir is not set, skip layout reorganize" >&2
        return 1
    fi
    local root="$install_dir"
    local share="$install_dir/share"
    local ver="$share/version"
    local cfg="$share/config"

    mkdir -p "$ver" "$cfg"

    if [ -d "$root/config" ]; then
        cp -a "$root/config/." "$cfg/"
        rm -rf "$root/config"
    fi

    if [ -d "$root/usr_msg_idl" ]; then
        if [ -e "$share/usr_msg_idl" ]; then
            rm -rf "$share/usr_msg_idl"
        fi
        mv "$root/usr_msg_idl" "$share/"
    fi

    shopt -s nullglob
    local f
    for f in "$root"/*.bash; do
        mv -f "$f" "$share/"
    done
    shopt -u nullglob

    if [ -f "$root/LICENSE" ]; then
        mv -f "$root/LICENSE" "$ver/"
    elif [ -f "$PROJECT_ROOT/LICENSE" ]; then
        cp -f "$PROJECT_ROOT/LICENSE" "$ver/LICENSE"
    fi

    shopt -s nullglob
    for f in "$root/msg_tool_version.txt" "$root/VERSION" "$root"/*version*.txt "$root"/*version*.json; do
        [ -f "$f" ] || continue
        mv -f "$f" "$ver/"
    done
    shopt -u nullglob
    if [ -f "$root/install_manifest.txt" ]; then
        mv -f "$root/install_manifest.txt" "$ver/"
    fi
    if [ ! -f "$ver/VERSION" ] && [ -f "$PROJECT_ROOT/VERSION" ]; then
        cp -f "$PROJECT_ROOT/VERSION" "$ver/VERSION"
    fi

    mkdir -p "$share/msg_libs"
    if [ -d "$root/msg_libs" ]; then
        local name
        for f in "$root/msg_libs"/*; do
            [ -e "$f" ] || continue
            name=$(basename "$f")
            if [ -L "$f" ]; then
                cp -L "$f" "$share/msg_libs/$name"
            else
                mv -f "$f" "$share/msg_libs/$name"
            fi
        done
        rm -rf "$root/msg_libs"
    fi

    rm -rf "$root/dist" "$root/tools" "$root/scripts"
}

function post_install_assets() {
    local share="$install_dir/share"
    local cfg="$share/config"

    mkdir -p "$share/version" "$cfg" "$share/msg_libs"

    reorganize_deploy_install_layout

    local deploy_dir="$PROJECT_ROOT/scripts/deploy"
    if [ -d "$deploy_dir/config" ]; then
        cp -a "$deploy_dir/config/." "$cfg/"
    fi
    if [ -f "$deploy_dir/segar_setup.bash" ]; then
        cp -f "$deploy_dir/segar_setup.bash" "$share/"
    fi

    # Runtime message libraries: copy real files (no symlinks in share/msg_libs)
    local dst_dir="$share/msg_libs"
    local config_file="$PROJECT_ROOT/runtime_msg_libs.txt"
    if [ -f "$config_file" ]; then
        local lib_name
        while IFS= read -r lib_name || [ -n "$lib_name" ]; do
            lib_name="${lib_name%%#*}"
            lib_name="$(echo "$lib_name" | xargs)"
            [ -n "$lib_name" ] || continue

            if [ -e "$install_dir/lib/$lib_name" ]; then
                cp -L "$install_dir/lib/$lib_name" "$dst_dir/$lib_name"
            else
                echo "Warning: runtime message library not found: $install_dir/lib/$lib_name" >&2
            fi
        done < "$config_file"
    fi

    # Second pass: anything still at install root (e.g. older deploy or extra unpack) is folded into share/.
    reorganize_deploy_install_layout
}

function deploy() {
    clear_dirs
    build_and_install
    post_install_assets
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    platform_name="${1:?usage: $0 <platform_name> [install_dir]}"
    install_dir="${2:-/opt/robot_lab/segar-workspace}"
    build_dir="$PROJECT_ROOT/tmp/deploy"
    echo "segar workspace will be installed to: $install_dir"
    deploy
fi
