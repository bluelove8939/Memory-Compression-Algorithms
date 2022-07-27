import os
import platform
import argparse


parser = argparse.ArgumentParser(description='Comparison Test Configs')
parser.add_argument('-cs', '--csize', default=64, help='Cache line size (int)', dest='csize')
parser.add_argument('-mi', '--maxiter', default=5000, help='Max iteration of the file fetch (int)', dest='maxiter')
comp_args, _ = parser.parse_known_args()

tb_name = 'tb_csv.exe'
if 'linux' in platform.platform().lower():
    tb_name = 'tb_csv'

os.system(f"gcc -o tb_csv ./tb_csv.c ./compression.c ./bdi_zerovec.c -lm -Wformat=0")

for model_name in os.listdir(os.path.join(os.curdir, 'extractions')):
    filelist_path = os.path.join(os.curdir, 'extractions', model_name, 'filelist.txt')
    result_path = os.path.join(os.curdir, 'extractions', model_name, 'comparison_results.csv')
    print(f"\n{tb_name} {filelist_path} 64 5000 {result_path}")
    os.system(f"{tb_name} {filelist_path} {comp_args.csize} {comp_args.maxiter} {result_path}")