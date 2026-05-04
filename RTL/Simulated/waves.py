import subprocess
import argparse
import os
import glob

def view(dir):
    # Check if wavefile exists
    if not os.path.exists(f'verilator/{dir}/build/waveform.vcd'):
        print(f"Error: No waveform file found for project '{dir}'.")
        return

    # Check if there is already a savefile
    savefile = glob.glob(f'verilator/{dir}/*.gtkw')

    try:
        # Run the command
        if len(savefile) == 0:
            subprocess.run(f'gtkwave build/waveform.vcd', cwd=f'verilator/{dir}', check=True, shell=True)
        else:
            savefile = savefile[0].split('\\')[-1].split('/')[-1]
            subprocess.run(f'gtkwave build/waveform.vcd -a {savefile}', cwd=f'verilator/{dir}', check=True, shell=True)
        print(f"Waveform viewer launched successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error launching waveform viewer: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='View the waveform of a verilator simulation.')
    parser.add_argument('dir', nargs=1, help='The name of the verilator project')

    args = parser.parse_args()

    view(args.dir[0])