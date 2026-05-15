import subprocess
import argparse
import os
import glob

def view(dir):
    # Check if wavefile exists
    if not os.path.exists(f'outputs/{dir}/waveform.fst'):
        print(f"Error: No waveform file found for project '{dir}'.")
        return

    # Check if there is already a savefile
    savefile = glob.glob(f'outputs/{dir}/*.gtkw')

    try:
        # Run the command
        if len(savefile) == 0:
            subprocess.run(f'gtkwave outputs/{dir}/waveform.fst', check=True, shell=True)
        else:
            savefile[0] = savefile[0].replace('\\', '/')
            subprocess.run(f'gtkwave outputs/{dir}/waveform.fst -a {savefile[0]}', check=True, shell=True)
        print(f"Waveform viewer launched successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error launching waveform viewer: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='View the waveform of a verilator simulation.')
    parser.add_argument('dir', nargs=1, help='The name of the verilator project')

    args = parser.parse_args()

    view(args.dir[0])