import os

for model_name in os.listdir(os.path.join(os.curdir, 'extractions')):
    filelist_path = os.path.join(os.curdir, 'extractions', model_name, 'filelist.txt')
    result_path = os.path.join(os.curdir, 'extractions', model_name, 'comparison_results.csv')
    print(f"\ntb_csv.exe {filelist_path} 64 5000 {result_path}")
    os.system(f"tb_csv.exe {filelist_path} 64 5000 {result_path}")