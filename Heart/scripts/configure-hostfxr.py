import sys
import os
import platform
import subprocess

if "DOTNET_SDK" not in os.environ:
    sys.stderr.write("DOTNET_SDK path not configured")
    exit(1)
SDK_PATH = os.environ["DOTNET_SDK"]
DOTNET_MIN_VER = "6.0"

def get_platform_info():
    system = platform.system()
    is_arm = "arm" in platform.version()
    is_64bit = sys.maxsize > 2**32
    return (system, is_arm, is_64bit)

def get_dotnet_version():
    highest_ver = ""
    out = subprocess.check_output(["dotnet", "--list-runtimes"])
    for line in out.splitlines():
        line = line.decode("utf-8")
        if not line.startswith("Microsoft.NETCore.App"):
            continue
        version_str = line.split(" ")[1]
        if not version_str.startswith(DOTNET_MIN_VER):
            continue
        if version_str > highest_ver:
            highest_ver = version_str
    return highest_ver

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
    hostfxr_dir = os.path.join(
        SDK_PATH,
        "host",
        "fxr",
        runtime_ver
    )

    sys.stdout.write(runtime_dir)
    sys.stderr.write(hostfxr_dir)

configure()