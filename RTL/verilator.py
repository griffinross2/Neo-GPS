import subprocess
import argparse
import os
import shutil

def build(dir, release=False):
    if not os.path.exists('build'):
        os.mkdir('build')

    try:
        # Run the command
        if release:
            subprocess.run(f'cmake -DCMAKE_BUILD_TYPE=Release .. -G "MinGW Makefiles"', cwd='build', check=True)
        else:
            subprocess.run(f'cmake .. -G "MinGW Makefiles"', cwd='build', check=True)
        subprocess.run(f'cmake --build . --target {dir}', cwd='build', check=True)
        print(f"Verilation successful.")
    except subprocess.CalledProcessError as e:
        print(f"Error during verilation: {e}")

def run(dir):
    # Ensure outputs exists
    if not os.path.exists('outputs'):
        os.mkdir('outputs')

    if not os.path.exists(f'outputs/{dir}'):
        os.mkdir(f'outputs/{dir}')

    try:
        # Run the command
        subprocess.run(f'build\\verilator\\{dir}\\{dir}.exe', check=True, shell=True)
        print(f"Simulation successful.")
    except subprocess.CalledProcessError as e:
        print(f"Error during simulation: {e}")

def clean(dir):
    shutil.rmtree('build')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Verilate a Verilog source file.')
    parser.add_argument('dir', nargs=1, help='The name of the verilator project')
    parser.add_argument('--build', action='store_true', help='Build the verilator project')
    parser.add_argument('--run', action='store_true', help='Run the verilator project')
    parser.add_argument('--clean', action='store_true', help='Clean the verilator project')
    parser.add_argument('--rebuild', action='store_true', help='Clean and build the verilator project')
    parser.add_argument('--all', action='store_true', help='Build and run the verilator project')
    parser.add_argument('--release', action='store_true', help='Build the verilator project in release mode')    

    args = parser.parse_args()

    if not any([args.build, args.run, args.clean, args.rebuild, args.all]):
        parser.error('No action specified, add --build or --run')
        
    if args.build:
        build(args.dir[0], release=args.release)

    if args.run:
        run(args.dir[0])

    if args.clean:
        clean(args.dir[0])

    if args.rebuild:
        clean(args.dir[0])
        build(args.dir[0], release=args.release)

    if args.all:
        build(args.dir[0], release=args.release)
        run(args.dir[0])
