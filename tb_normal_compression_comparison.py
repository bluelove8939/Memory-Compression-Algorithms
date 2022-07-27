import os
import subprocess
import platform
import argparse


parser = argparse.ArgumentParser(description='Comparison Test Configs')
parser.add_argument('-cs', '--csize', default=64, help='Cache line size (int)', dest='csize')
parser.add_argument('-mi', '--maxiter', default=5000, help='Max iteration of the file fetch (int)', dest='maxiter')
comp_args, _ = parser.parse_known_args()

tb_name = 'tb_csv.exe'
if 'linux' in platform.platform().lower():
    tb_name = './tb_csv'

print(f"gcc -o tb_csv ./tb_csv.c ./compression.c ./bdi_zerovec.c -lm -Wformat=0")
subprocess.run(f"gcc -o tb_csv ./tb_csv.c ./compression.c ./bdi_zerovec.c -lm -Wformat=0", shell=True, check=True)
#
# out = subprocess.run(f"tb_csv.exe "
#                f"{os.path.join(os.curdir, 'extractions', 'ResNet50_Imagenet', 'filelist.txt')} "
#                f"64 5000 "
#                f"{os.path.join(os.curdir, 'extractions', 'ResNet50_Imagenet', 'comparison_results.csv')}")
# print(out)

for model_name in os.listdir(os.path.join(os.curdir, 'extractions')):
    filelist_path = os.path.join(os.curdir, 'extractions', model_name, 'filelist.txt')
    result_path = os.path.join(os.curdir, 'extractions', model_name, 'comparison_results.csv')
    print(f"\n{tb_name} {filelist_path} {comp_args.csize} {comp_args.maxiter} {result_path}")
    subprocess.run(f"{tb_name} {filelist_path} {comp_args.csize} {comp_args.maxiter} {result_path}")