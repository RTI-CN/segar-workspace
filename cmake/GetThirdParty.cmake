# GetThirdParty.cmake
# Download third-party archives from the Nexus artifact repository

# Embedded Python script source (inside CMake)
set(GET_3RD_PARTY_PYTHON_SCRIPT "
#!/usr/bin/env python3
import argparse
import os
import shutil
import tarfile
import sys
import subprocess

sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)

def check_library_installed(manifest_file, library_name):
    if not os.path.exists(manifest_file):
        return False
    try:
        with open(manifest_file, 'r', encoding='utf-8') as f:
            content = f.read()
            if f\"{library_name} \" in content:
                return True
    except Exception:
        pass
    return False

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
        if os.path.isdir(src_path):
            if os.path.exists(dst_path) and os.path.isdir(dst_path):
                # Destination directory exists; merge recursively
                merge_directory(src_path, dst_path)
            else:
                # Destination directory missing; copy the entire directory
                if os.path.exists(dst_path):
                    shutil.rmtree(dst_path)
                shutil.copytree(src_path, dst_path)
        else:
            # File entry: copy directly
            if os.path.exists(dst_path):
                os.remove(dst_path)
            shutil.copy2(src_path, dst_path)

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

def download_file(url, local_path):
    print(f\"Downloading from {url}...\", flush=True)
    result = subprocess.run(
        ['curl', '-L', '-f', '--progress-bar', '--show-error', '-o', local_path, url],
        stderr=sys.stderr
    )
    if result.returncode != 0:
        raise RuntimeError(f\"Failed to download {url}\")
    print(f\"Download completed: {local_path}\", flush=True)

def install_from_local(library_name, platform_name, version, local_path, 
                       download_dir, extract_dir, source_dir=None):
    if extract_dir is None:
        if source_dir is None:
            raise RuntimeError(\"Either extract_dir or source_dir must be provided\")
        extract_dir = os.path.join(source_dir, 'install', platform_name)
    manifest_file = os.path.join(extract_dir, \"install_manifest.txt\")
    # Check whether already installed
    if check_library_installed(manifest_file, library_name):
        print(f\"Library {library_name} already installed (found in manifest)\")
        return
    package_name = f\"{library_name}_{platform_name}_{version}.tgz\"
    local_package_path = os.path.join(local_path, package_name)
    if not os.path.exists(local_package_path):
        raise RuntimeError(f\"Package not found at local path: {local_package_path}\")
    print(f\"Using local package for library: {library_name} (version: {version})\")
    print(f\"  Local package: {local_package_path}\")
    third_party_dir = os.path.join(extract_dir, \"third_party\")
    manifest_file = os.path.join(extract_dir, \"install_manifest.txt\")
    print(f\"  Target: {third_party_dir}\")
    os.makedirs(extract_dir, exist_ok=True)
    print(f\"Extracting {package_name} from local path to temporary directory...\")
    with tarfile.open(local_package_path, 'r:gz') as tar:
        tar.extractall(path=extract_dir)
    extracted_root_dir = find_extracted_directory(extract_dir, library_name, platform_name, version)
    print(f\"Found extracted directory: {extracted_root_dir}\")
    os.makedirs(third_party_dir, exist_ok=True)
    print(f\"Moving contents to {third_party_dir}...\")
    merge_directory(extracted_root_dir, third_party_dir)
    if os.path.exists(extracted_root_dir):
        shutil.rmtree(extracted_root_dir)
    print(f\"Installation from local path completed: {third_party_dir}\")
    print(\"Updating install manifest...\")
    update_manifest(manifest_file, library_name, version, f\"LOCAL:{local_path}\")
    print(f\"Added to manifest: {library_name} {version} (local)\")
    print(f\"Library {library_name} ready at: {third_party_dir}\")

def install_from_nexus(library_name, platform_name, version, 
                       download_dir, extract_dir, source_dir=None):
    if download_dir is None:
        if source_dir is None:
            raise RuntimeError(\"Either download_dir or source_dir must be provided\")
        download_dir = os.path.join(source_dir, 'install', platform_name)
    if extract_dir is None:
        if source_dir is None:
            raise RuntimeError(\"Either extract_dir or source_dir must be provided\")
        extract_dir = os.path.join(source_dir, 'install', platform_name)
    manifest_file = os.path.join(extract_dir, \"install_manifest.txt\")
    # Check whether already installed
    if check_library_installed(manifest_file, library_name):
        print(f\"Library {library_name} already installed (found in manifest)\")
        return
    package_name = f\"{library_name}_{platform_name}_{version}.tgz\"
    nexus_base_url = \"https://gitee.com/RTI3/segar-sdk/releases/download/release/\"
    download_url = f\"{nexus_base_url}/{package_name}\"
    local_package_path = os.path.join(download_dir, package_name)
    print(f\"Downloading library: {library_name} (version: {version})\")
    print(f\"  Package: {package_name}\")
    print(f\"  URL: {download_url}\")
    print(f\"  Download to: {local_package_path}\")
    os.makedirs(download_dir, exist_ok=True)
    # Check whether package file exists and is valid
    need_download = True
    if os.path.exists(local_package_path):
        # Validate archive integrity
        try:
            with tarfile.open(local_package_path, 'r:gz') as tar:
                tar.getmembers()  # Try listing entries to verify integrity
            print(f\"Package already exists and is valid: {local_package_path}\")
            need_download = False
        except (tarfile.TarError, IOError, EOFError):
            print(f\"Package file is corrupted, will re-download: {local_package_path}\")
            os.remove(local_package_path)
    if need_download:
        download_file(download_url, local_package_path)
    third_party_dir = os.path.join(extract_dir, \"third_party\")
    print(f\"  Target: {third_party_dir}\")
    os.makedirs(extract_dir, exist_ok=True)
    print(f\"Extracting {package_name} to temporary directory...\")
    with tarfile.open(local_package_path, 'r:gz') as tar:
        tar.extractall(path=extract_dir)
    extracted_root_dir = find_extracted_directory(extract_dir, library_name, platform_name, version)
    print(f\"Found extracted directory: {extracted_root_dir}\")
    os.makedirs(third_party_dir, exist_ok=True)
    print(f\"Moving contents to {third_party_dir}...\")
    merge_directory(extracted_root_dir, third_party_dir)
    if os.path.exists(extracted_root_dir):
        shutil.rmtree(extracted_root_dir)
    if os.path.exists(local_package_path):
        os.remove(local_package_path)
        print(f\"Removed package file: {local_package_path}\")
    print(f\"Installation completed: {third_party_dir}\")
    print(\"Updating install manifest...\")
    update_manifest(manifest_file, library_name, version, download_url)
    print(f\"Added to manifest: {library_name} {version}\")
    print(f\"Library {library_name} ready at: {third_party_dir}\")

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

def install_msg_tool_wheel(platform_name, source_dir):
    third_party_dir = os.path.join(source_dir, 'install', platform_name, 'third_party')
    dist_dir = os.path.join(third_party_dir, 'dist')
    install_script = os.path.join(third_party_dir, 'scripts', 'install_wheel.sh')
    # Do not skip based on version_file: msg_tool extraction brings version_file, and -ra full reinstall still needs wheel installation
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
        print(f\"Warning: Failed to install msg_tool wheel: {result.stderr}\")
    else:
        print(f\"  msg_tool wheel installed successfully\")

def load_dependencies(platform_name, deps_file, source_dir):
    if os.path.isabs(deps_file):
        full_deps_file = deps_file
    else:
        full_deps_file = os.path.join(source_dir, deps_file)
    if not os.path.exists(full_deps_file):
        print(f\"Warning: Dependencies file not found: {full_deps_file}\")
        return 1
    print(f\"Loading dependencies from: {full_deps_file} (platform: {platform_name})\")
    extract_dir = os.path.join(source_dir, 'install', platform_name)
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
                version_file = os.path.join(extract_dir, 'third_party', 'msg_tool_version.txt')
                if not os.path.exists(version_file):
                    install_msg_tool = True
            try:
                if lib_local_path:
                    print(f\"Processing dependency: {lib_name} {platform_name} {lib_version} (local: {lib_local_path})\")
                    install_from_local(
                        lib_name, platform_name, lib_version,
                        lib_local_path, None, extract_dir, source_dir
                    )
                else:
                    print(f\"Processing dependency: {lib_name} {platform_name} {lib_version}\")
                    install_from_nexus(
                        lib_name, platform_name, lib_version,
                        None, extract_dir, source_dir
                    )
            except Exception as e:
                print(f\"ERROR: Failed to install {lib_name}: {e}\", file=sys.stderr)
                return 1
    if install_msg_tool:
        install_msg_tool_wheel(platform_name, source_dir)
    install_python_dependencies(['pandas', 'numpy', 'matplotlib'])
    print(\"All dependencies loaded\", flush=True)
    return 0

def install_dependencies(platform_name, target_dir, source_dir):
    \"\"\"Install dependencies to target path (copy direct files from third_party root and direct files from lib/scripts/bin).\"\"\"
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
            if os.path.isfile(src_path):
                shutil.copy2(src_path, dst_path)
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
        os.makedirs(dst_dir, exist_ok=True)
        # Copy direct files only; skip subdirectories
        try:
            items = os.listdir(src_dir)
            for item in items:
                if item in ('.', '..'):
                    continue
                src_path = os.path.join(src_dir, item)
                dst_path = os.path.join(dst_dir, item)
                if os.path.isfile(src_path):
                    shutil.copy2(src_path, dst_path)
                    print(f\"  Copied: {dir_name}/{item}\")
        except OSError as e:
            print(f\"Warning: Failed to copy {dir_name}: {e}\")
    print(f\"Installation completed: {target_dir}\")
    return 0

def main():
    parser = argparse.ArgumentParser(description='Third-party dependencies management')
    subparsers = parser.add_subparsers(dest='command', help='Command to execute')
    # load command: load dependencies
    load_parser = subparsers.add_parser('load', help='Load dependencies from file')
    load_parser.add_argument('platform_name', help='Platform name (e.g., x86_64, orin)')
    load_parser.add_argument('deps_file', help='Dependencies file path (e.g., depend_libs.txt)')
    load_parser.add_argument('--source-dir', required=True, help='CMake source directory')
    load_parser.add_argument('--output-dir-file', help='File to write third_party directory path')
    # install command: install dependencies to target path
    install_parser = subparsers.add_parser('install', help='Install dependencies to target directory')
    install_parser.add_argument('platform_name', help='Platform name (e.g., x86_64, orin)')
    install_parser.add_argument('target_dir', help='Target installation directory')
    install_parser.add_argument('--source-dir', required=True, help='CMake source directory')
    args = parser.parse_args()
    if args.command == 'load':
        result = load_dependencies(args.platform_name, args.deps_file, args.source_dir)
        if args.output_dir_file:
            extract_dir = os.path.join(args.source_dir, 'install', args.platform_name)
            third_party_dir = os.path.join(extract_dir, 'third_party')
            with open(args.output_dir_file, 'w') as f:
                f.write(third_party_dir)
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
    find_program(PYTHON3_EXECUTABLE python3 REQUIRED)
    
    # Create temporary Python script file
    set(PYTHON_SCRIPT_FILE "${CMAKE_BINARY_DIR}/.get_3rd_party_${PLATFORM_NAME}.py")
    file(WRITE "${PYTHON_SCRIPT_FILE}" "${GET_3RD_PARTY_PYTHON_SCRIPT}")
    
    # Stream logs in real time (no output capture)
    set(THIRD_PARTY_DIR_FILE "${CMAKE_BINARY_DIR}/.third_party_dir_${PLATFORM_NAME}.txt")
    execute_process(
        COMMAND ${PYTHON3_EXECUTABLE} -u ${PYTHON_SCRIPT_FILE} load ${PLATFORM_NAME} ${DEPS_FILE} --source-dir ${CMAKE_SOURCE_DIR} --output-dir-file ${THIRD_PARTY_DIR_FILE}
        RESULT_VARIABLE PYTHON_RESULT
        ERROR_VARIABLE PYTHON_ERROR
    )
    
    # Read installation directory path from file
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
    
    # Add to PATH
    set(THIRD_PARTY_BIN_DIR "${CMAKE_SOURCE_DIR}/install/${PLATFORM_NAME}/third_party/bin")
    if(EXISTS "${THIRD_PARTY_BIN_DIR}")
        list(PREPEND CMAKE_PROGRAM_PATH "${THIRD_PARTY_BIN_DIR}")
        set(CMAKE_PROGRAM_PATH "${CMAKE_PROGRAM_PATH}" PARENT_SCOPE)
        set(ENV{PATH} "${THIRD_PARTY_BIN_DIR}:$ENV{PATH}")
    endif()
endfunction()

# Function: install dependencies to target path
# Parameters:
#   PLATFORM_NAME - platform name (e.g., x86_64, orin)
#   TARGET_DIR - target installation directory
function(install_dependencies PLATFORM_NAME TARGET_DIR)
    find_program(PYTHON3_EXECUTABLE python3 REQUIRED)
    
    # Create temporary Python script file
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
