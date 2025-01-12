import sys
import os
import shutil
import platform
import subprocess
from pathlib import Path

DOTNET_BIN = shutil.which("dotnet")
DOTNET_MIN_VER = "8.0"

if DOTNET_BIN is None:
    sys.stderr.write("'dotnet' executable not found, try adding to PATH")
    exit(1)


SDK_PATH = Path(DOTNET_BIN).parent

def get_platform_info():
    system = platform.system()
    is_arm = "arm" in platform.version().lower()
    is_64bit = sys.maxsize > 2**32
    return (system, is_arm, is_64bit)

def get_dotnet_version():
    highest_ver = (0, 0, 0)
    out = subprocess.check_output(["dotnet", "--list-runtimes"])
    for line in out.splitlines():
        line = line.decode("utf-8")
        if not line.startswith("Microsoft.NETCore.App"):
            continue
        version_str = line.split(" ")[1]
        if not version_str.startswith(DOTNET_MIN_VER):
            continue
        versions = tuple(int(v) for v in version_str.split("."))
        if versions > highest_ver:
            highest_ver = versions
    return ".".join(str(s) for s in highest_ver)

def get_dotnet_runtime_string(platform_info):
    sys_map = {
        "Windows": "win",
        "Linux": "linux",
        "macOS": "osx",
        "Darwin": "osx"
    }

    # is_arm, is_64bit
    arch_map = {
        (False, False): "x86",
        (False, True): "x64",
        (True, False): "arm",
        (True, True): "arm64" 
    }

    runtime_str = "%s-%s" % (
        sys_map[platform_info[0]],
        arch_map[(platform_info[1], platform_info[2])]
    )

    return runtime_str

def get_hostfxr_name(platform_info):
    sys_map = {
        "Windows": "hostfxr.dll",
        "Linux": "hostfxr.so",
        "macOS": "libhostfxr.dylib",
        "Darwin": "libhostfxr.dylib"
    }
    
    return sys_map[platform_info[0]]

def configure():
    platform_info = get_platform_info()
    runtime_ver = get_dotnet_version()
    runtime_str = get_dotnet_runtime_string(platform_info)
    
    if (runtime_ver == ""):
        print("Could not find acceptable installed version of dotnet, must be at least %s" % DOTNET_MIN_VER)

    runtime_dir = os.path.join(
        SDK_PATH,
        "packs",
        "Microsoft.NETCore.App.Host." + runtime_str,
        runtime_ver,
        "runtimes",
        runtime_str,
        "native"
    )
    
    hostfxr_path = os.path.join(
        SDK_PATH,
        "host",
        "fxr",
        runtime_ver,
        get_hostfxr_name(platform_info)
    )

    sys.stdout.write(runtime_dir + ";" + hostfxr_path + ";" + runtime_str)

if __name__ == "__main__":
    configure()
