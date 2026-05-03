import subprocess
import argparse
import os
import shutil

def build(dir):
    if not os.path.exists(f'verilator/{dir}/build'):
        os.mkdir(f'verilator/{dir}/build')

    try:
        # Run the command
        subprocess.run('cmake ..', cwd=f'verilator/{dir}/build', check=True)
        subprocess.run('cmake --build .', cwd=f'verilator/{dir}/build', check=True)
        print(f"Verilation successful.")
    except subprocess.CalledProcessError as e:
        print(f"Error during verilation: {e}")

def run(dir):
    try:
        # Run the command
        subprocess.run(f'Debug\\{dir}.exe', cwd=f'verilator/{dir}/build', check=True, shell=True)
        print(f"Simulation successful.")
    except subprocess.CalledProcessError as e:
        print(f"Error during simulation: {e}")

def clean(dir):
    shutil.rmtree(f'verilator/{dir}/build')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Verilate a Verilog source file.')
    parser.add_argument('dir', nargs=1, help='The name of the verilator project')
    parser.add_argument('--build', action='store_true', help='Build the verilator project')
    parser.add_argument('--run', action='store_true', help='Run the verilator project')
    parser.add_argument('--clean', action='store_true', help='Clean the verilator project')
    parser.add_argument('--rebuild', action='store_true', help='Clean and build the verilator project')
    parser.add_argument('--all', action='store_true', help='Build and run the verilator project')

    args = parser.parse_args()

    if not any([args.build, args.run, args.clean, args.rebuild, args.all]):
        parser.error('No action specified, add --build or --run')
        
    if args.build:
        build(args.dir[0])

    if args.run:
        run(args.dir[0])

    if args.clean:
        clean(args.dir[0])

    if args.rebuild:
        clean(args.dir[0])
        build(args.dir[0])

    if args.all:
        build(args.dir[0])
        run(args.dir[0])
