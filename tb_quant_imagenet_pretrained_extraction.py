import os
import argparse
import platform
import subprocess

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader

import torchvision.transforms as transforms
import torchvision.datasets as datasets

from models.tools.imagenet_utils.args_generator import args
from models.model_presets import imagenet_pretrained
from models.tools.extractor import QuantModelExtractor, weight_trace, bias_trace
from models.tools.quanitzation import QuantizationModule


parser = argparse.ArgumentParser(description='Comparison Test Configs')
parser.add_argument('-cs', '--csize', default=64, help='Cache line size (int)', dest='csize')
parser.add_argument('-mi', '--maxiter', default=5000, help='Max iteration of the file fetch (int)', dest='maxiter')
comp_args, _ = parser.parse_known_args()


device = "cuda" if torch.cuda.is_available() else "cpu"
print(f"Using {device} device")


# Dataset configuration
dataset_dirname = args.data

train_dataset = datasets.ImageFolder(
        os.path.join(dataset_dirname, 'train'),
        transforms.Compose([
            transforms.RandomResizedCrop(224),
            transforms.RandomHorizontalFlip(),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
        ]))

test_dataset = datasets.ImageFolder(
        os.path.join(dataset_dirname, 'val'),
        transforms.Compose([
            transforms.Resize(256),
            transforms.CenterCrop(224),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
        ]))

if args.distributed:
    train_sampler = torch.utils.data.distributed.DistributedSampler(train_dataset)
    val_sampler = torch.utils.data.distributed.DistributedSampler(test_dataset, shuffle=False, drop_last=True)
else:
    train_sampler = None
    val_sampler = None

train_loader = torch.utils.data.DataLoader(
    train_dataset, batch_size=args.batch_size, shuffle=(train_sampler is None),
    num_workers=args.workers, pin_memory=True, sampler=train_sampler)

test_loader = torch.utils.data.DataLoader(
    test_dataset, batch_size=args.batch_size, shuffle=False,
    num_workers=args.workers, pin_memory=True, sampler=val_sampler)


if __name__ == '__main__':
    save_dirpath = os.path.join(os.curdir, 'model_output')
    os.makedirs(save_dirpath, exist_ok=True)

    tb_name = 'tb_csv.exe'
    if 'linux' in platform.platform().lower():
        tb_name = './tb_csv'

    print(f"gcc -o tb_csv ./tb_csv.c ./compression.c ./bdi_zerovec.c -lm -Wformat=0")
    subprocess.run(f"gcc -o tb_csv ./tb_csv.c ./compression.c ./bdi_zerovec.c -lm -Wformat=0", shell=True, check=True)

    for model_type, model_config in imagenet_pretrained.items():
        full_modelname = f"{model_type}_quant_Imagenet"
        save_modelname = f"{model_type}_quant_Imagenet.pth"
        save_fullpath = os.path.join(save_dirpath, save_modelname)

        print("Test Configs:")
        print(f"- full modelname: {full_modelname}")
        print(f"- save modelname: {save_modelname}")
        print(f"- save fullpath:  {save_fullpath}\n")

        model = model_config.generate()
        lr = 0.0001
        optimizer = optim.Adam(model.parameters(), lr=lr)
        loss_fn = nn.CrossEntropyLoss().to(device)
        qmodule = QuantizationModule(train_loader, loss_fn=loss_fn, optimizer=optimizer)
        quant_model = qmodule.quantize(model, verbose=1, device='cpu')
        torch.save(quant_model.state_dict(), save_fullpath)

        save_extraction_dir = os.path.join(os.curdir, 'extractions_quant', full_modelname)
        os.makedirs(save_extraction_dir, exist_ok=True)

        print(f"extracting '{full_modelname}' at {save_extraction_dir}")

        extractor_module = QuantModelExtractor(target_model=quant_model, output_modelname=full_modelname)
        extractor_module.add_param_trace(weight_trace)  # add weight trace
        extractor_module.add_param_trace(bias_trace)    # add bias trace
        extractor_module.extract_params()                           # extract paramters
        extractor_module.save_params(savepath=save_extraction_dir)  # save extracted parameters

        print(f"extracting '{full_modelname}' completed")
        print(f"generating comparison test results")

        filelist_path = os.path.join(os.curdir, 'extractions', full_modelname, 'filelist.txt')
        result_path = os.path.join(os.curdir, 'extractions', full_modelname, 'comparison_results.csv')
        print(f"\n{tb_name} {filelist_path} {comp_args.csize} {comp_args.maxiter} {result_path}")
        tb_result = subprocess.run(f"{tb_name} {filelist_path} {comp_args.csize} {comp_args.maxiter} {result_path}",
                                   shell=True)

        if tb_result.returncode != 0:
            print('Error occurred on running compression algorithm testbench')
        else:
            print(f"compression algorithm comparison test completed\n")

