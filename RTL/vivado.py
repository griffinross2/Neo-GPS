import os
import sys
import subprocess

if __name__ == "__main__":
    if(len(sys.argv) < 2):
        print("Usage: python simulate.py <toplevel>")
        print("Options:")
        print("-s : Synthesize before simulating")
        print("-g : GUI mode")
        sys.exit(1)
    args = sys.argv[1:]

    # GUI mode?
    if "-g" in args:
        gui = True
        args.remove("-g")
    else:
        gui = False

    toplevel = args[0]
    
    # Find TCL script
    if not os.path.exists("scripts/{}.tcl".format(toplevel)):
        print("TCL file not found!")
        sys.exit(1)

    # Run Vivado
    cmd_1 = "call \"settings64.bat\""
    cmd_2 = "vivado -mode {mode} -source ../scripts/{toplevel}.tcl".format(mode="gui" if gui else "batch", toplevel=toplevel)

    subprocess.run(cmd_1 + " && " + cmd_2, cwd="vivado", shell=True, check=True)
    print("Simulation complete.")
