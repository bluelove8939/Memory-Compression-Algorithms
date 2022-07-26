import copy
import torch
import torch.quantization.quantize_fx as quantize_fx

from models.tools.progressbar import progressbar


class QuantizationModule(object):
    def __init__(self, tuning_dataloader, loss_fn, optimizer):
        self.tuning_dataloader = tuning_dataloader
        self.loss_fn = loss_fn
        self.optimizer = optimizer

    def quantize(self, model, default_qconfig='fbgemm', calib=True, verbose=2, device="auto"):
        if device == "auto":
            device = "cuda" if torch.cuda.is_available() else "cpu"

        print("\nQuantization Configs")
        print(f"- loss_fn: {self.loss_fn}")
        print(f"- qconfig: {default_qconfig}")
        print(f"- device:  {device}")

        quant_model = copy.deepcopy(model)
        quant_model.eval()
        qconfig = torch.quantization.get_default_qconfig(default_qconfig)
        qconfig_dict = {"": qconfig}

        if verbose: print("preparing quantization (symbolic tracing)")
        model_prepared = quantize_fx.prepare_fx(quant_model, qconfig_dict)
        if verbose == 2: print(model_prepared)

        if calib: self.calibrate(model_prepared, verbose=verbose, device=device)
        model_quantized = quantize_fx.convert_fx(model_prepared)
        return model_quantized

    def calibrate(self, model, verbose=2, device="auto"):
        if verbose == 1:
            print(f'\r{progressbar(0, len(self.tuning_dataloader), 50)}'
                  f'  calibration iter: {0:3d}/{len(self.tuning_dataloader):3d}', end='')
        elif verbose:
            print(f'calibration iter: {0:3d}/{len(self.tuning_dataloader):3d}')

        cnt = 1
        model.eval()                                      # set to evaluation mode

        with torch.no_grad():                             # do not save gradient when evaluation mode
            for image, target in self.tuning_dataloader:  # extract input and output data
                image = image.to(device)
                model(image)                              # forward propagation

                if verbose == 1:
                    print(f'\r{progressbar(cnt, len(self.tuning_dataloader), 50)}'
                          f'  calibration iter: {cnt:3d}/{len(self.tuning_dataloader):3d}', end='')
                elif verbose:
                    print(f'calibration iter: {cnt:3d}/{len(self.tuning_dataloader):3d}', end='')

                cnt += 1

        if verbose == 1: print('\n')
        elif verbose: print()
