import sys
import os
import shutil
import platform
import subprocess
import argparse
import zipfile
from pathlib import Path


def get_platform_info():
    system = platform.system()
    is_arm = "arm" in platform.version().lower()
    is_64bit = sys.maxsize > 2**32
    return (system, is_arm, is_64bit)


platform_info = get_platform_info()
is_unix = platform_info[0] != "Windows"


def zip_directory_with_symlinks(directory, zip_filename):
    """Zips a directory while preserving symbolic links."""
    assert is_unix
    root_dir_name = os.path.basename(os.path.abspath(directory))
    with zipfile.ZipFile(zip_filename, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(directory, followlinks=False):
            for name in files + dirs:
                filepath = os.path.join(root, name)
                arcname = os.path.join(root_dir_name, os.path.relpath(filepath, directory))
                if os.path.islink(filepath):
                    link_target = os.readlink(filepath)
                    zip_info = zipfile.ZipInfo(arcname)
                    zip_info.create_system = 3
                    zip_info.external_attr = 0o120777 << 16
                    zipf.writestr(zip_info, link_target)
                else:
                    zipf.write(filepath, arcname)


def bundle_macos(out_dir, build_dir):
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    # Copy bundle to out directory
    print("Copying Editor.app...")
    shutil.copytree(f"{build_dir}/Editor.app", f"{out_dir}/Editor.app", symlinks=False, dirs_exist_ok=True)

    # Copy resource files
    print("Copying Resource Files...")
    resource_dir = f"{out_dir}/Editor.app/Contents/Resources"
    shutil.copytree(f"{build_dir}/resources", f"{resource_dir}/resources", symlinks=False, dirs_exist_ok=True)
    shutil.copytree(f"{build_dir}/scripting", f"{resource_dir}/scripting", symlinks=False, dirs_exist_ok=True)
    shutil.copytree(f"{build_dir}/templates", f"{resource_dir}/templates", symlinks=False, dirs_exist_ok=True)

    # Copy shared libraries
    print("Copying Shared Libraries...")
    framework_dir = f"{out_dir}/Editor.app/Contents/Frameworks"
    if not os.path.exists(framework_dir):
        os.makedirs(framework_dir)
    shutil.copy(f"{build_dir}/libMoltenVK.dylib", framework_dir)
    shutil.copy(f"{build_dir}/libshaderc_shared.1.dylib", framework_dir)
    shutil.copy(f"{build_dir}/libspirv-cross-c-shared.0.dylib", framework_dir)
    symlink_path = f"{framework_dir}/libvulkan.1.dylib"
    if not os.path.exists(symlink_path):
        os.symlink("libMoltenVK.dylib", symlink_path)

    # Zip final result
    print("Zipping bundle...")
    zip_directory_with_symlinks(f"{out_dir}/Editor.app", f"{out_dir}/Editor.zip")


def package_editor(args):
    print("\n\n======== Configuring Project ========\n\n")

    configure_script = "ConfigureDistDebug.bat" if args.debug else "ConfigureDistRelease.bat"
    if is_unix:
        out = subprocess.check_output(["sh", configure_script])
    else:
        out = subprocess.check_output([configure_script])

    print("\n\n======== Building Project ========\n\n")

    build_script = "BuildDebug.bat" if args.debug else "BuildRelease.bat"
    if is_unix:
        out = subprocess.check_output(["sh", build_script])
    else:
        out = subprocess.check_output([build_script])

    print("\n\n======== Bundling Project Files ========\n\n")

    out_dir = f"../build/Packaged/{'Debug' if args.debug else 'Release'}"
    build_dir = f"../build/bin/{'Debug' if args.debug else 'Release'}"
    if platform_info[0] == "Windows":
        raise NotImplementedError
    elif platform_info[0] == "Linux":
        raise NotImplementedError
    else:
        bundle_macos(out_dir, build_dir)

    print("\n\n======== Success ========\n\n")
    print(f"Wrote results to {out_dir}\n\n")


if __name__ == "__main__":
    print("Ensure this script is run inside the 'scripts' dir")

    parse = argparse.ArgumentParser()
    parse.add_argument(
        "--debug",
        type=int,
        default=0,
    )
    args = parse.parse_args()

    package_editor(args)
