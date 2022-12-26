import subprocess
import sys

subprocess.run([
    "dotnet",
    "publish",
    sys.argv[1] + "/Heart.NET.Sdk/Heart.NET.Sdk.csproj",
    "-r", "osx-arm64",
    "-c", "Release",
    "--self-contained"
])