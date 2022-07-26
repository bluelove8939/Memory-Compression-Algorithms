import torch
import matplotlib.pyplot as plt


AUTO = 'auto'
ALL = 'all'
DONT_SAVE = 'dont_save'


class ActivationImgGenerator(object):
    def __init__(self, model_savepath, device=AUTO):
        self.model_savepath = model_savepath
        self.device = device  # defined target device (mainly 'cpu' or 'cuda')
        self._activation = {}
        self._hook_handlers = {}

        if self.device == AUTO:
            self.device = "cuda" if torch.cuda.is_available() else "cpu"

    def get_activation_hook(self, name, channel_size):
        def hook(model, layer_input, layer_output):
            self._activation[name] = layer_output.detach()[0][:channel_size]

        return hook

    def add_trace(self, layer: torch.nn.Module, name: str, channel_size: int=9):
        if name not in self._hook_handlers:
            self._hook_handlers[name] = layer.register_forward_hook(self.get_activation_hook(name, channel_size))
        else:
            print(f"Hook '{name}' already exists")

    def remove_trace(self, name: str=ALL):
        if name == ALL:
            for key in self._hook_handlers.keys():
                self._hook_handlers[key].remove()
                del self._hook_handlers[key]
        else:
            self._hook_handlers[name].remove()
            del self._hook_handlers[name]

    def show_activations(self, test_dataset, model: torch.nn.Module, img_savepath:str=DONT_SAVE):
        model.load_state_dict(torch.load(self.model_savepath))
        activation = {}

        data, _ = test_dataset[0]
        data.unsqueeze_(0)
        model.eval()
        model(data.to(self.device))

        rgrid, cgrid = 0, 0
        for key in self._activation.keys():
            rgrid = max(rgrid, self._activation[key].squeeze().size(0))
            cgrid += 1

        fig, axs = plt.subplots(cgrid, rgrid, figsize=(4 * rgrid, 4 * cgrid), gridspec_kw={'width_ratios': [1] * rgrid})
        fig.suptitle("Intermediate Activation Images")

        ridx, cidx = 0, 0
        for key in self._activation.keys():
            act = self._activation[key].squeeze()
            for ridx in range(rgrid):
                if ridx < act.size(0):
                    axs[cidx, ridx].imshow(act[ridx].to('cpu'))
                    axs[cidx, ridx].set_title(f"{key}_channel{ridx}")
                else:
                    axs[cidx, ridx].axis('off')
            cidx += 1

        # plt.tight_layout()
        plt.show()
        if img_savepath != DONT_SAVE:
            plt.savefig(img_savepath)