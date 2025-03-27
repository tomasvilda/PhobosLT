import os
import gzip
import shutil
from glob import glob

Import("env")

def gzip_files(source, target, env):
    # Configuration
    project_dir = env["PROJECT_DIR"]
    data_dir = os.path.join(project_dir, env.GetProjectOption("board_build.littlefs_dir", "data"))
    build_dir = env.subst("$BUILD_DIR")
    littlefs_dir = os.path.join(build_dir, "littlefs")
    
    print(f"Preparing LittleFS files from {data_dir} to {littlefs_dir}")
    
    # Create destination directory if it doesn't exist
    if os.path.exists(littlefs_dir):
        shutil.rmtree(littlefs_dir, ignore_errors=True)        
    
    os.makedirs(littlefs_dir)
    
    # Process all files in the data directory
    for root, dirs, files in os.walk(data_dir):
        # Create corresponding directories in build/littlefs
        relative_path = os.path.relpath(root, data_dir)
        dest_path = os.path.join(littlefs_dir, relative_path)
        
        if relative_path != "." and not os.path.exists(dest_path):
            os.makedirs(dest_path)
        
        # Process each file
        for file in files:
            source_file = os.path.join(root, file)
            dest_file = os.path.join(dest_path, file)
            
            # shutil.copy2(source_file, dest_file)
            
            # Compress the file
            gz_file = dest_file + '.gz'
            print(f"Compressing: {source_file} -> {gz_file}")
            
            with open(source_file, 'rb') as f_in:
                with gzip.open(gz_file, 'wb', compresslevel=9) as f_out:
                    shutil.copyfileobj(f_in, f_out)
            
# Hook our function before the standard buildfs runs
env.AddPreAction("$BUILD_DIR/littlefs.bin", gzip_files)
