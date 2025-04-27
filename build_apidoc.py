


import os
import subprocess

# Define the output directory and the location of the Doxyfile
output_dir = "build/doc"
doxyfile_path = "doc/Doxyfile"

def create_output_directory():
    """Ensure that the output directory exists, create it if necessary."""
    if not os.path.exists(output_dir):
        print(f"Output directory '{output_dir}' does not exist. Creating...")
        os.makedirs(output_dir)
    else:
        print(f"Output directory '{output_dir}' already exists.")

def generate_apidoc():
    """Run Doxygen to generate API documentation."""
    print(f"Running Doxygen on '{doxyfile_path}'...")
    result = subprocess.run(["doxygen", doxyfile_path], capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"Doxygen failed with error: {result.stderr}")
    else:
        print(f"Doxygen completed successfully. Documentation is available in '{output_dir}'.")

if __name__ == "__main__":
    create_output_directory()
    generate_apidoc()

