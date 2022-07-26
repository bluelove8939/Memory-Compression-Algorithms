import os

import torch
import torch.nn as nn
import torch.optim as optim
import torch.nn.functional as F
from torch.utils.data import DataLoader

import torchvision
import torchvision.transforms as transforms
import torchvision.datasets as datasets

from tools.imagenet_utils.args_generator import args
from tools.imagenet_utils.training import validate
from tools.activation_imgshow import ActivationImgGenerator


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


# Model configuration
model_type = 'ResNet50'
dataset_type = 'Imagenet'
weight_preset = torchvision.models.ResNet50_Weights.IMAGENET1K_V1
model = torchvision.models.resnet50(weights=weight_preset).to(device)

lr = 0.0001
optimizer = optim.Adam(model.parameters(), lr=lr)
loss_fn = nn.CrossEntropyLoss().to(device)


save_dirpath = os.path.join(os.curdir, 'model_output')
if not os.path.exists(save_dirpath):
    os.makedirs(save_dirpath)
save_modelname = f"{model_type}_{dataset_type}.pth"
save_fullpath = os.path.join(save_dirpath, save_modelname)


if __name__ == '__main__':
    validate(test_loader, model, loss_fn, args)
    torch.save(model.state_dict(), save_fullpath)

    img_dirname = os.path.join(os.curdir, 'activation_images')
    os.makedirs(img_dirname, exist_ok=True)
    img_filename = f"{model_type}_{dataset_type}_activation.png"
    img_generator = ActivationImgGenerator(save_fullpath, device=device)
    img_generator.add_trace(model.conv1, name='conv1', channel_size=9)
    img_generator.add_trace(model.layer1, name='layer1', channel_size=9)
    img_generator.add_trace(model.layer2, name='layer2', channel_size=9)
    img_generator.add_trace(model.layer3, name='layer3', channel_size=9)
    img_generator.add_trace(model.layer4, name='layer4', channel_size=9)
    img_generator.show_activations(test_dataset, model, img_savepath=os.path.join(img_dirname, img_filename))