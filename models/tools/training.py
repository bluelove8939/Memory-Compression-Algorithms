import sys
import torch
import torch.distributed as dist
from models.tools.progressbar import progressbar


# device = "cuda" if torch.cuda.is_available() else "cpu"
# print(f"Using {device} device")


class AverageMeter(object):
    def __init__(self, name, fmt=':f'):
        self.name = name
        self.fmt = fmt
        self.reset()

    def reset(self):
        self.val = 0
        self.avg = 0
        self.sum = 0
        self.count = 0

    def update(self, val, n=1):
        self.val = val
        self.sum += val * n
        self.count += n
        self.avg = self.sum / self.count

    def all_reduce(self):
        total = torch.FloatTensor([self.sum, self.count])
        dist.all_reduce(total, dist.ReduceOp.SUM, async_op=False)
        self.sum, self.count = total.tolist()
        self.avg = self.sum / self.count

    def __str__(self):
        fmtstr = '{name} {val' + self.fmt + '} ({avg' + self.fmt + '})'
        return fmtstr.format(**self.__dict__)


def train(dataloader, model, loss_fn, optimizer, verbose=1, savelog_path=None, device='auto'):
    if device == 'auto':
        device = "cuda" if torch.cuda.is_available() else "cpu"

    size = len(dataloader.dataset)  # size of the dataset
    lossmeter = AverageMeter('loss', fmt=':.4f')

    model.train()                   # turn the model into train mode
    for batch, (X, y) in enumerate(dataloader):  # each index of dataloader will be batch index
        X, y = X.to(device), y.to(device)        # extract input and output

        # Compute prediction error
        pred = model(X)          # predict model
        # print(pred.shape, y.shape)
        loss = loss_fn(pred, y)  # calculate loss

        # Backpropagation
        optimizer.zero_grad()  # gradient initialization (just because torch accumulates gradient)
        loss.backward()        # backward propagate with the loss value (or vector)
        optimizer.step()       # update parameters

        loss, current = loss.item(), (batch+1) * dataloader.batch_size
        lossmeter.update(loss, dataloader.batch_size)

        if verbose == 1:
            sys.stdout.write(f"\rtrain status: {progressbar(current, size, scale=50)} {current / size * 100:3.0f}%  "
                             f"{lossmeter} [{current:>5d}/{size:>5d}]")
        elif verbose:
            sys.stdout.write(f"train status: {progressbar(current, size, scale=50)} {current / size * 100:3.0f}%  "
                             f"{lossmeter}  [{current:>5d}/{size:>5d}]\n")

    if savelog_path is not None:
        with open(savelog_path, 'at') as logfile:
            logfile.write(f"train status: {progressbar(size, size, scale=50)} 100%  "
                          f"{lossmeter}  [{size:>5d}/{size:>5d}]\n")

    if verbose == 1: print('')


def test(dataloader, model, loss_fn, verbose=1, savelog_path=None, device='auto'):
    if device == 'auto':
        device = "cuda" if torch.cuda.is_available() else "cpu"

    size = len(dataloader.dataset)  # dataset size
    num_batches = len(dataloader)   # the number of batches
    model.eval()                    # convert model into evaluation mode
    test_loss, correct = 0, 0       # check total loss and count correctness
    with torch.no_grad():           # set all of the gradient into zero
        for didx, (X, y) in enumerate(dataloader):
            X, y = X.to(device), y.to(device)     # extract input and output
            pred = model(X)                       # predict with the given model
            test_loss += loss_fn(pred, y).item()  # acculmulate total loss
            correct += (pred.argmax(1) == y).type(torch.float).sum().item()  # count correctness

            if verbose == 1:
                sys.stdout.write(f"\rtest status:  {progressbar(didx+1, len(dataloader), scale=50)} "
                                 f"{(didx+1) / len(dataloader) * 100:3.0f}%")
            elif verbose:
                sys.stdout.write(f"test status:  {progressbar(didx+1, len(dataloader), scale=50)} "
                                 f"{(didx+1) / len(dataloader) * 100:3.0f}%\n")

    test_loss /= num_batches   # make an average of the total loss
    correct /= size            # make an average with correctness count

    if verbose:
        if verbose == 1: print()
        sys.stdout.write(f"Accuracy: {(100*correct):>0.1f}%, Avg loss: {test_loss:>8f}\n")

    if savelog_path is not None:
        with open(savelog_path, 'at') as logfile:
            logfile.write(f"Accuracy: {(100*correct):>0.1f}%, Avg loss: {test_loss:>8f}\n")

    return 100 * correct, test_loss