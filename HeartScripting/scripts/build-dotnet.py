import subprocess
import sys
from multiprocessing import Process
import os

def run_process():
    subprocess.run([
        "dotnet",
        "build",
        sys.argv[1] + "/Heart.NET.Sdk/Heart.NET.Sdk.csproj",
    ])
    
if __name__ == '__main__':
    print(os.environ)
    p = Process(target=run_process)#, args=(sys.argv[1],))
    p.start()
    p.join()