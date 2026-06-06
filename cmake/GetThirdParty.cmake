# GetThirdParty.cmake
# Download third-party archives from the Nexus artifact repository

# This module's directory. Do not use CMAKE_CURRENT_LIST_DIR inside function(): it follows the
# caller (e.g. scripts/deploy), not this file — file(COPY) would miss RtiSegarConfig.cmake.
get_filename_component(_GET_THIRD_PARTY_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)

# Embedded Python script source (inside CMake)
set(GET_3RD_PARTY_PYTHON_SCRIPT "
#!/usr/bin/env python3
import argparse
import os
import platform
import shutil
import tarfile
import sys
import subprocess

sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)

def get_manifest_entry(manifest_file, library_name):
    if not os.path.exists(manifest_file):
        return None
    try:
        with open(manifest_file, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line.startswith(f\"{library_name} \"):
                    return line
    except Exception:
        pass
    return None

def check_library_installed(manifest_file, library_name, expected_source_info=None):
    entry = get_manifest_entry(manifest_file, library_name)
    if entry is None:
        return False
    if expected_source_info is None:
        return True
    return entry.endswith(expected_source_info)

def find_extracted_directory(extract_dir, library_name, platform_name, version):
    extracted_root_dir = os.path.join(extract_dir, f\"{library_name}_{platform_name}_{version}\")
    if os.path.exists(extracted_root_dir) and os.path.isdir(extracted_root_dir):
        return extracted_root_dir
    extracted_root_dir = os.path.join(extract_dir, f\"{library_name}_{version}\")
    if os.path.exists(extracted_root_dir) and os.path.isdir(extracted_root_dir):
        return extracted_root_dir
    possible_dirs = []
    for item in os.listdir(extract_dir):
        item_path = os.path.join(extract_dir, item)
        if (os.path.isdir(item_path) and 
            not item.startswith('.temp_') and 
            item != 'third_party'):
            possible_dirs.append(item_path)
    if len(possible_dirs) == 1:
        return possible_dirs[0]
    elif len(possible_dirs) == 0:
        raise RuntimeError(f\"Extracted directory not found in: {extract_dir}\")
    else:
        raise RuntimeError(f\"Extracted directory ambiguous (found {len(possible_dirs)} directories): {possible_dirs}\")

def _parse_version_tuple(version_str):
    if not version_str:
        return (0, 0, 0)
    parts = str(version_str).strip().replace('-', '.').split('.')
    nums = []
    for p in parts[:3]:
        digits = ''.join(c for c in p if c.isdigit())
        nums.append(int(digits) if digits else 0)
    while len(nums) < 3:
        nums.append(0)
    return tuple(nums)

def get_nexus_base_url_for_dependency_version(version_str):
    \"\"\"
    Use the dependency version from depend_libs (not the msg_tool root version):
    <= 2.5.0 -> .../download/release
    > 2.5.0  -> .../download/{version}/
    \"\"\"
    if not version_str:
        return 'https://gitee.com/RTI3/segar-sdk/releases/download/release'
    t = _parse_version_tuple(version_str)
    if t <= (2, 5, 0):
        return 'https://gitee.com/RTI3/segar-sdk/releases/download/release'
    return f'https://gitee.com/RTI3/segar-sdk/releases/download/{version_str.strip()}'

def _remove_dst_for_copy(dst_path):
    if not os.path.exists(dst_path):
        return
    if os.path.isdir(dst_path) and not os.path.islink(dst_path):
        shutil.rmtree(dst_path)
    else:
        os.remove(dst_path)

def copy_entry_preserving_symlinks(src_path, dst_path):
    \"\"\"Copy files/directories/symlinks while preserving common .so -> .so.x links in lib. Do not dereference symlinks into regular files.\"\"\"
    _remove_dst_for_copy(dst_path)
    if os.path.islink(src_path):
        shutil.copy2(src_path, dst_path, follow_symlinks=False)
    elif os.path.isdir(src_path):
        shutil.copytree(src_path, dst_path, symlinks=True)
    else:
        shutil.copy2(src_path, dst_path)

def merge_directory(src, dst):
    os.makedirs(dst, exist_ok=True)
    try:
        items = os.listdir(src)
    except OSError:
        return
    for item in items:
        if item in ('.', '..'):
            continue
        src_path = os.path.join(src, item)
        dst_path = os.path.join(dst, item)
        if os.path.isdir(src_path) and not os.path.islink(src_path):
            if os.path.exists(dst_path) and os.path.isdir(dst_path) and not os.path.islink(dst_path):
                # Destination directory exists; merge recursively
                merge_directory(src_path, dst_path)
            else:
                copy_entry_preserving_symlinks(src_path, dst_path)
        else:
            copy_entry_preserving_symlinks(src_path, dst_path)

def update_manifest(manifest_file, library_name, version, source_info):
    content = \"\"
    if os.path.exists(manifest_file):
        try:
            with open(manifest_file, 'r', encoding='utf-8') as f:
                content = f.read()
        except Exception:
            pass
    lines = content.split('\\n')
    new_lines = []
    for line in lines:
        if line.strip() and not line.startswith(f\"{library_name} \"):
            new_lines.append(line)
    new_lines.append(f\"{library_name} {version} {source_info}\")
    with open(manifest_file, 'w', encoding='utf-8') as f:
        f.write('\\n'.join(new_lines))
        if new_lines:
            f.write('\\n')

def resolve_manifest_path(target_dir):
    p_share = os.path.join(target_dir, 'share', 'version', 'install_manifest.txt')
    p_root = os.path.join(target_dir, 'install_manifest.txt')
    if os.path.isfile(p_share):
        return p_share
    if os.path.isfile(p_root):
        return p_root
    return p_root

def download_file(url, local_path):
    print(f\"Downloading from {url}...\", flush=True)
    result = subprocess.run(
        [
            'curl',
            '-L',
            '-f',
            '--retry', '5',
            '--retry-all-errors',
            '--retry-delay', '2',
            '--connect-timeout', '15',
            '--continue-at', '-',
            '--progress-bar',
            '--show-error',
            '-o', local_path,
            url,
        ],
        stderr=sys.stderr
    )
    if result.returncode != 0:
        raise RuntimeError(f\"Failed to download {url}\")
    print(f\"Download completed: {local_path}\", flush=True)

def install_from_local(library_name, platform_name, version, local_path, 
                       download_dir, extract_dir, source_dir=None, install_dir=None):
    if extract_dir is None:
        if source_dir is None:
            raise RuntimeError(\"Either extract_dir or source_dir must be provided\")
        extract_dir = os.path.join(source_dir, 'install', platform_name)
    
    # Determine target directory based on install_dir parameter
    if install_dir is not None:
        target_dir = install_dir
    else:
        target_dir = os.path.join(extract_dir, \"third_party\")
    
    manifest_file = resolve_manifest_path(target_dir)
    download_platform_name = platform_name
    if library_name == 'msg_tool' and platform_name == 'orin' and platform.machine() == 'aarch64':
        download_platform_name = 'native_orin'
    package_name = f'{library_name}_{download_platform_name}_{version}.tgz'
    local_package_path = os.path.join(local_path, package_name)
    if not os.path.exists(local_package_path):
        raise RuntimeError(f\"Package not found at local path: {local_package_path}\")
    local_package_mtime_ns = os.stat(local_package_path).st_mtime_ns
    local_source_info = f\"LOCAL:{local_path}:{local_package_mtime_ns}\"
    if check_library_installed(manifest_file, library_name, local_source_info):
        print(f\"Library {library_name} already installed (local package unchanged)\")
        return
    print(f\"Using local package for library: {library_name} (version: {version})\")
    print(f\"  Local package: {local_package_path}\")
    print(f\"  Target: {target_dir}\")
    os.makedirs(extract_dir, exist_ok=True)
    os.makedirs(target_dir, exist_ok=True)
    print(f\"Extracting {package_name} from local path to temporary directory...\")
    with tarfile.open(local_package_path, 'r:gz') as tar:
        tar.extractall(path=extract_dir)
    extracted_root_dir = find_extracted_directory(extract_dir, library_name, platform_name, version)
    print(f\"Found extracted directory: {extracted_root_dir}\")
    print(f\"Moving contents to {target_dir}...\")
    merge_directory(extracted_root_dir, target_dir)
    if os.path.exists(extracted_root_dir):
        shutil.rmtree(extracted_root_dir)
    print(f\"Installation from local path completed: {target_dir}\")
    print(\"Updating install manifest...\")
    update_manifest(manifest_file, library_name, version, local_source_info)
    print(f\"Added to manifest: {library_name} {version} (local)\")
    print(f\"Library {library_name} ready at: {target_dir}\")

def install_from_nexus(library_name, platform_name, version, 
                       download_dir, extract_dir, source_dir=None, install_dir=None):
    if download_dir is None:
        if source_dir is None:
            raise RuntimeError(\"Either download_dir or source_dir must be provided\")
        download_dir = os.path.join(source_dir, 'install', platform_name)
    if extract_dir is None:
        if source_dir is None:
            raise RuntimeError(\"Either extract_dir or source_dir must be provided\")
        extract_dir = os.path.join(source_dir, 'install', platform_name)
    
    # Determine target directory based on install_dir parameter
    if install_dir is not None:
        target_dir = install_dir
    else:
        target_dir = os.path.join(extract_dir, \"third_party\")
    
    manifest_file = resolve_manifest_path(target_dir)
    # Check if already installed
    if check_library_installed(manifest_file, library_name):
        print(f\"Library {library_name} already installed (found in manifest)\")
        return
    download_platform_name = platform_name
    if library_name == 'msg_tool' and platform_name == 'orin' and platform.machine() == 'aarch64':
        download_platform_name = 'native_orin'
    package_name = f'{library_name}_{download_platform_name}_{version}.tgz'
    nexus_base_url = get_nexus_base_url_for_dependency_version(version)
    download_url = f\"{nexus_base_url.rstrip('/')}/{package_name}\"
    local_package_path = os.path.join(download_dir, package_name)
    print(f\"Downloading library: {library_name} (version: {version})\")
    print(f\"  Package: {package_name}\")
    print(f\"  URL: {download_url}\")
    print(f\"  Download to: {local_package_path}\")
    print(f\"  Target: {target_dir}\")
    os.makedirs(download_dir, exist_ok=True)
    os.makedirs(target_dir, exist_ok=True)
    # Check whether the archive exists and is valid
    need_download = True
    if os.path.exists(local_package_path):
        # Validate archive integrity
        try:
            with tarfile.open(local_package_path, 'r:gz') as tar:
                tar.getmembers()  # Read member list to verify integrity
            print(f\"Package already exists and is valid: {local_package_path}\")
            need_download = False
        except (tarfile.TarError, IOError, EOFError):
            print(f\"Package file is corrupted, will re-download: {local_package_path}\")
            os.remove(local_package_path)
    if need_download:
        download_file(download_url, local_package_path)
    print(f\"Extracting {package_name} to temporary directory...\")
    with tarfile.open(local_package_path, 'r:gz') as tar:
        tar.extractall(path=extract_dir)
    extracted_root_dir = find_extracted_directory(extract_dir, library_name, platform_name, version)
    print(f\"Found extracted directory: {extracted_root_dir}\")
    print(f\"Moving contents to {target_dir}...\")
    merge_directory(extracted_root_dir, target_dir)
    if os.path.exists(extracted_root_dir):
        shutil.rmtree(extracted_root_dir)
    if os.path.exists(local_package_path):
        os.remove(local_package_path)
        print(f\"Removed package file: {local_package_path}\")
    print(f\"Installation completed: {target_dir}\")
    print(\"Updating install manifest...\")
    update_manifest(manifest_file, library_name, version, download_url)
    print(f\"Added to manifest: {library_name} {version}\")
    print(f\"Library {library_name} ready at: {target_dir}\")

def install_python_dependencies(packages=None):
    if packages is None:
        packages = ['pandas', 'numpy', 'matplotlib']
    print(f\"Installing Python dependencies: {', '.join(packages)}...\")
    import shutil
    pip3_path = shutil.which('pip3')
    if not pip3_path:
        packages_str = ' '.join(packages)
        print(f\"Warning: pip3 not found, skipping Python dependencies installation\")
        print(f\"Warning: You may need to manually install: pip3 install --user {packages_str}\")
        return
    result = subprocess.run(
        [pip3_path, 'install', '--user'] + packages,
        capture_output=True,
        text=True
    )
    if result.returncode != 0:
        packages_str = ' '.join(packages)
        print(f\"Warning: Failed to install Python dependencies: {result.stderr}\")
        print(f\"Warning: You may need to manually install: pip3 install --user {packages_str}\")
    else:
        print(\"Python dependencies installed successfully\")

def install_msg_tool_wheel(platform_name, source_dir, install_dir=None):
    # Determine target directory based on install_dir parameter
    if install_dir is not None:
        third_party_dir = install_dir
    else:
        third_party_dir = os.path.join(source_dir, 'install', platform_name, 'third_party')
    
    # Normalize to absolute paths to avoid cwd-dependent relative path issues
    third_party_dir = os.path.abspath(third_party_dir)
    dist_dir = os.path.join(third_party_dir, 'dist')
    install_script = os.path.join(third_party_dir, 'scripts', 'install_wheel.sh')
    # Do not skip based on version_file: extracting msg_tool already carries version_file, and -ra full reinstall must also run wheel installation
    if not os.path.exists(dist_dir):
        print(f\"Warning: msg_tool dist directory not found: {dist_dir}\")
        return
    import glob
    whl_files = glob.glob(os.path.join(dist_dir, 'msg_tool-*.whl'))
    if not whl_files:
        print(f\"Warning: No msg_tool wheel files found in: {dist_dir}\")
        return
    latest_whl = max(whl_files, key=os.path.getmtime)
    whl_name = os.path.basename(latest_whl)
    whl_relative = f\"dist/{whl_name}\"
    print(f\"Installing msg_tool from wheel package...\")
    print(f\"  Found wheel file: {latest_whl}\")
    if not os.path.exists(install_script):
        print(f\"Warning: install_wheel.sh script not found: {install_script}\")
        return
    result = subprocess.run(
        ['bash', install_script, whl_relative, '-r'],
        cwd=third_party_dir,
        capture_output=True,
        text=True
    )
    if result.returncode != 0:
        raise RuntimeError(
            \"Failed to install msg_tool wheel. \"
            f\"cmd=bash {install_script} {whl_relative} -r, \"
            f\"cwd={third_party_dir}, \"
            f\"returncode={result.returncode}, \"
            f\"stderr={result.stderr}, \"
            f\"stdout={result.stdout}\"
        )
    else:
        print(f\"  msg_tool wheel installed successfully\")

def load_dependencies(platform_name, deps_file, source_dir, install_dir=None):
    if os.path.isabs(deps_file):
        full_deps_file = deps_file
    else:
        full_deps_file = os.path.join(source_dir, deps_file)
    if not os.path.exists(full_deps_file):
        print(f\"Warning: Dependencies file not found: {full_deps_file}\")
        return 1
    
    extract_dir = os.path.join(source_dir, 'install', platform_name)
    
    # Determine target directory for manifest file
    if install_dir is not None:
        target_dir = install_dir
    else:
        target_dir = os.path.join(extract_dir, \"third_party\")
    
    print(f\"Loading dependencies from: {full_deps_file} (platform: {platform_name})\")
    if install_dir is not None:
        print(f\"Using custom install directory: {install_dir}\")
    
    install_msg_tool = False
    with open(full_deps_file, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split()
            if len(parts) < 2:
                print(f\"Warning: Invalid dependency format: {line}\")
                continue
            lib_name = parts[0]
            lib_version = parts[1]
            lib_local_path = parts[2] if len(parts) > 2 else None
            if lib_name == 'msg_tool':
                version_file = os.path.join(target_dir, 'msg_tool_version.txt')
                if not os.path.exists(version_file):
                    install_msg_tool = True
            try:
                if lib_local_path:
                    print(f\"Processing dependency: {lib_name} {platform_name} {lib_version} (local: {lib_local_path})\")
                    install_from_local(
                        lib_name, platform_name, lib_version,
                        lib_local_path, None, extract_dir, source_dir, install_dir
                    )
                else:
                    print(f\"Processing dependency: {lib_name} {platform_name} {lib_version}\")
                    install_from_nexus(
                        lib_name, platform_name, lib_version,
                        None, extract_dir, source_dir, install_dir
                    )
            except Exception as e:
                print(f\"ERROR: Failed to install {lib_name}: {e}\", file=sys.stderr)
                return 1
    if install_msg_tool:
        install_msg_tool_wheel(platform_name, source_dir, install_dir)
    install_python_dependencies(['pandas', 'numpy', 'matplotlib'])
    print(\"All dependencies loaded\", flush=True)
    return 0

def install_dependencies(platform_name, target_dir, source_dir):
    \"\"\"Install dependencies to target path (files in third_party root and direct files in lib/scripts/bin).\"\"\"
    third_party_dir = os.path.join(source_dir, 'install', platform_name, 'third_party')
    if not os.path.exists(third_party_dir):
        raise RuntimeError(f\"Third-party directory not found: {third_party_dir}\")
    print(f\"Installing dependencies from {third_party_dir} to {target_dir}\")
    os.makedirs(target_dir, exist_ok=True)
    # Install direct files under third_party root (e.g., msg_tool_version.txt, setup.bash, LICENSE)
    try:
        for item in os.listdir(third_party_dir):
            if item in ('.', '..'):
                continue
            src_path = os.path.join(third_party_dir, item)
            dst_path = os.path.join(target_dir, item)
            if os.path.isfile(src_path) or os.path.islink(src_path):
                if os.path.isdir(src_path):
                    continue
                copy_entry_preserving_symlinks(src_path, dst_path)
                print(f\"  Copied: {item}\")
    except OSError as e:
        print(f\"Warning: Failed to copy root files: {e}\")
    # Directories to install
    dirs_to_install = ['lib', 'scripts', 'bin']
    for dir_name in dirs_to_install:
        src_dir = os.path.join(third_party_dir, dir_name)
        dst_dir = os.path.join(target_dir, dir_name)
        if not os.path.exists(src_dir):
            print(f\"Warning: Source directory not found: {src_dir}\")
            continue
        try:
            merge_directory(src_dir, dst_dir)
            print(f\"  Merged: {dir_name}/\")
        except OSError as e:
            print(f\"Warning: Failed to copy {dir_name}: {e}\")
    install_runtime_message_libraries(target_dir, source_dir)
    mirror_runtime_message_libraries_to_prefix(target_dir, source_dir)
    print(f\"Installation completed: {target_dir}\")
    return 0

def load_runtime_message_libraries(source_dir):
    config_path = os.path.join(source_dir, 'runtime_msg_libs.txt')
    if not os.path.exists(config_path):
        return []
    libs = []
    with open(config_path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            libs.append(line)
    return libs

def install_runtime_message_libraries(target_dir, source_dir):
    libs = load_runtime_message_libraries(source_dir)
    if not libs:
        return
    lib_dir = os.path.join(target_dir, 'lib')
    msg_lib_dir = os.path.join(target_dir, 'msg_libs')
    os.makedirs(msg_lib_dir, exist_ok=True)
    for entry in os.listdir(msg_lib_dir):
        entry_path = os.path.join(msg_lib_dir, entry)
        if os.path.isdir(entry_path) and not os.path.islink(entry_path):
            shutil.rmtree(entry_path)
        else:
            os.unlink(entry_path)
    for lib_name in libs:
        src_path = os.path.join(lib_dir, lib_name)
        dst_path = os.path.join(msg_lib_dir, lib_name)
        if not os.path.lexists(src_path):
            print(f\"Warning: runtime message library not found: {src_path}\")
            continue
        if os.path.lexists(dst_path):
            os.unlink(dst_path)
        shutil.copy2(src_path, dst_path, follow_symlinks=True)
        print(f\"  Runtime msg lib: msg_libs/{lib_name} (copied from {src_path})\")

def mirror_runtime_message_libraries_to_prefix(target_dir, source_dir):
    \"\"\"Also copy runtime msg libs under <parent>/msg_libs/ (parent = dirname(target_dir)), e.g. output/msg_libs when target_dir is output/third_party.\"\"\"
    libs = load_runtime_message_libraries(source_dir)
    if not libs:
        return
    parent = os.path.dirname(os.path.abspath(target_dir))
    parent_msg = os.path.join(parent, 'msg_libs')
    lib_dir = os.path.join(target_dir, 'lib')
    os.makedirs(parent_msg, exist_ok=True)
    for lib_name in libs:
        src_path = os.path.join(lib_dir, lib_name)
        dst_path = os.path.join(parent_msg, lib_name)
        if not os.path.lexists(src_path):
            print(f\"Warning: runtime message library not found for prefix msg_libs mirror: {src_path}\")
            continue
        if os.path.lexists(dst_path):
            os.unlink(dst_path)
        shutil.copy2(src_path, dst_path, follow_symlinks=True)
        print(f\"  Runtime msg lib (install prefix): msg_libs/{lib_name} (copied from {src_path})\")

def main():
    parser = argparse.ArgumentParser(description='Third-party dependencies management')
    subparsers = parser.add_subparsers(dest='command', help='Command to execute')
    # load command: load dependencies
    load_parser = subparsers.add_parser('load', help='Load dependencies from file')
    load_parser.add_argument('platform_name', help='Platform name (e.g., x86_64, orin)')
    load_parser.add_argument('deps_file', help='Dependencies file path (e.g., depend_libs.txt)')
    load_parser.add_argument('--source-dir', required=True, help='CMake source directory')
    load_parser.add_argument('--install-dir', help='Custom install directory (optional)')
    load_parser.add_argument('--output-dir-file', help='File to write third_party directory path')
    # install command: install dependencies to target path
    install_parser = subparsers.add_parser('install', help='Install dependencies to target directory')
    install_parser.add_argument('platform_name', help='Platform name (e.g., x86_64, orin)')
    install_parser.add_argument('target_dir', help='Target installation directory')
    install_parser.add_argument('--source-dir', required=True, help='CMake source directory')
    args = parser.parse_args()
    if args.command == 'load':
        result = load_dependencies(args.platform_name, args.deps_file, args.source_dir, args.install_dir)
        if args.output_dir_file:
            if args.install_dir:
                target_dir = args.install_dir
            else:
                target_dir = os.path.join(args.source_dir, 'install', args.platform_name, 'third_party')
            with open(args.output_dir_file, 'w') as f:
                f.write(target_dir)
        return result
    elif args.command == 'install':
        return install_dependencies(args.platform_name, args.target_dir, args.source_dir)
    else:
        parser.print_help()
        return 1

if __name__ == '__main__':
    sys.exit(main())
")

function(load_dependencies PLATFORM_NAME DEPS_FILE)
    # Parse optional INSTALL_DIR argument
    set(options)
    set(oneValueArgs INSTALL_DIR)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    find_program(PYTHON3_EXECUTABLE python3 REQUIRED)
    
    # Create a temporary Python script file
    set(PYTHON_SCRIPT_FILE "${CMAKE_BINARY_DIR}/.get_3rd_party_${PLATFORM_NAME}.py")
    file(WRITE "${PYTHON_SCRIPT_FILE}" "${GET_3RD_PARTY_PYTHON_SCRIPT}")
    
    # Prepare command arguments
    set(PYTHON_COMMAND_ARGS 
        ${PYTHON3_EXECUTABLE} -u ${PYTHON_SCRIPT_FILE} load ${PLATFORM_NAME} ${DEPS_FILE} 
        --source-dir ${CMAKE_SOURCE_DIR}
    )
    
    # Add install_dir argument if provided
    if(ARG_INSTALL_DIR)
        list(APPEND PYTHON_COMMAND_ARGS --install-dir ${ARG_INSTALL_DIR})
    endif()
    
    set(THIRD_PARTY_DIR_FILE "${CMAKE_BINARY_DIR}/.third_party_dir_${PLATFORM_NAME}.txt")
    list(APPEND PYTHON_COMMAND_ARGS --output-dir-file ${THIRD_PARTY_DIR_FILE})
    
    # Stream logs in real time (do not capture output)
    execute_process(
        COMMAND ${PYTHON_COMMAND_ARGS}
        RESULT_VARIABLE PYTHON_RESULT
        ERROR_VARIABLE PYTHON_ERROR
    )
    
    # Read install prefix path from file
    if(EXISTS "${THIRD_PARTY_DIR_FILE}")
        file(READ "${THIRD_PARTY_DIR_FILE}" THIRD_PARTY_DIR)
        string(STRIP "${THIRD_PARTY_DIR}" THIRD_PARTY_DIR)
        if(THIRD_PARTY_DIR)
            set(THIRD_PARTY_DIR "${THIRD_PARTY_DIR}" PARENT_SCOPE)
        endif()
        file(REMOVE "${THIRD_PARTY_DIR_FILE}")
    endif()
    
    # Clean up temporary script file
    file(REMOVE "${PYTHON_SCRIPT_FILE}")
    
    if(NOT PYTHON_RESULT EQUAL 0)
        message(FATAL_ERROR "load_dependencies failed: ${PYTHON_ERROR}")
    endif()
    
    # Add PATH
    if(ARG_INSTALL_DIR)
        set(THIRD_PARTY_BIN_DIR "${ARG_INSTALL_DIR}/bin")
    else()
        set(THIRD_PARTY_BIN_DIR "${CMAKE_SOURCE_DIR}/install/${PLATFORM_NAME}/third_party/bin")
    endif()
    
    if(EXISTS "${THIRD_PARTY_BIN_DIR}")
        list(PREPEND CMAKE_PROGRAM_PATH "${THIRD_PARTY_BIN_DIR}")
        set(CMAKE_PROGRAM_PATH "${CMAKE_PROGRAM_PATH}" PARENT_SCOPE)
        set(ENV{PATH} "${THIRD_PARTY_BIN_DIR}:$ENV{PATH}")
    endif()

    # find_package(RtiSegar): 与 lib/cmake/SegarTransform/ 同层约定
    if(ARG_INSTALL_DIR)
        set(_RTI_SEGAR_INSTALL_PREFIX "${ARG_INSTALL_DIR}")
    else()
        set(_RTI_SEGAR_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install/${PLATFORM_NAME}/third_party")
    endif()
    set(_RTI_SEGAR_CFG_DIR "${_RTI_SEGAR_INSTALL_PREFIX}/lib/cmake/RtiSegar")
    file(MAKE_DIRECTORY "${_RTI_SEGAR_CFG_DIR}")
    file(COPY "${_GET_THIRD_PARTY_CMAKE_DIR}/RtiSegarConfig.cmake"
         DESTINATION "${_RTI_SEGAR_CFG_DIR}")
endfunction()

# Function: install dependencies to target path
# Parameters:
#   PLATFORM_NAME - platform name (e.g., x86_64, orin)
#   TARGET_DIR - target install directory
function(install_dependencies PLATFORM_NAME TARGET_DIR)
    find_program(PYTHON3_EXECUTABLE python3 REQUIRED)
    
    # Create a temporary Python script file
    set(PYTHON_SCRIPT_FILE "${CMAKE_BINARY_DIR}/.get_3rd_party_${PLATFORM_NAME}.py")
    file(WRITE "${PYTHON_SCRIPT_FILE}" "${GET_3RD_PARTY_PYTHON_SCRIPT}")
    
    # Execute install command
    execute_process(
        COMMAND ${PYTHON3_EXECUTABLE} -u ${PYTHON_SCRIPT_FILE} install ${PLATFORM_NAME} ${TARGET_DIR} --source-dir ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE PYTHON_RESULT
        ERROR_VARIABLE PYTHON_ERROR
    )
    
    # Clean up temporary script file
    file(REMOVE "${PYTHON_SCRIPT_FILE}")
    
    if(NOT PYTHON_RESULT EQUAL 0)
        message(FATAL_ERROR "install_dependencies failed: ${PYTHON_ERROR}")
    endif()
endfunction()
