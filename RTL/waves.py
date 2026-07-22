import subprocess
import argparse
import os
import glob

def view_gtkw(dir, wavefile='waveform.fst'):
    # Check if wavefile exists
    if not os.path.exists(f'outputs/{dir}/{wavefile}'):
        print(f"Error: No waveform file found for project '{dir}'.")
        return

    # Check if there is already a savefile
    savefile = glob.glob(f'outputs/{dir}/*.gtkw')

    try:
        # Run the command
        if len(savefile) == 0:
            subprocess.run(f'gtkwave outputs/{dir}/{wavefile}', check=True, shell=True)
        else:
            savefile[0] = savefile[0].replace('\\', '/')
            subprocess.run(f'gtkwave outputs/{dir}/{wavefile} -a {savefile[0]}', check=True, shell=True)
        print(f"Waveform viewer launched successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error launching waveform viewer: {e}")

def view_surfer(dir, wavefile='waveform.fst'):
    # Check if wavefile exists
    if not os.path.exists(f'outputs/{dir}/{wavefile}'):
        print(f"Error: No waveform file found for project '{dir}'.")
        return

    # Check if there is already a savefile
    savefile = glob.glob(f'outputs/{dir}/*.surf.ron')

    try:
        # Run the command
        if len(savefile) == 0:
            subprocess.run(f'surfer outputs/{dir}/{wavefile}', check=True, shell=True)
        else:
            savefile[0] = savefile[0].replace('\\', '/')
            subprocess.run(f'surfer outputs/{dir}/{wavefile} -s {savefile[0]}', check=True, shell=True)
        print(f"Waveform viewer launched successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error launching waveform viewer: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='View the waveform of a verilator simulation.')
    parser.add_argument('dir', nargs=1, help='The name of the verilator project')
    parser.add_argument('--wavefile', nargs=1, help='Waveform file name (default: waveform.fst)', default=['waveform.fst'])
    parser.add_argument('--gtkw', action='store_true', help='Use GTKWave instead of Surfer')

    args = parser.parse_args()

    if args.gtkw:
        view_gtkw(args.dir[0], wavefile=args.wavefile[0])
    else:
        view_surfer(args.dir[0], wavefile=args.wavefile[0])