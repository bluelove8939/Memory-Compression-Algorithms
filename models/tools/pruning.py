import torch
import torch.nn.utils.prune as prune

from models.tools.training import train, test


class PruneModule(object):
    def __init__(self, tuning_dataloader, loss_fn, optimizer):
        self.tuning_dataloader = tuning_dataloader
        self.loss_fn = loss_fn
        self.optimizer = optimizer

    def remove_prune_model(self, module: torch.nn.Module):
        for sub_idx, sub_module in module._modules.items():
            if isinstance(sub_module, torch.nn.Conv2d):
                prune.remove(sub_module, 'weight')
            # elif isinstance(sub_module, torch.nn.Linear) or isinstance(sub_module, torch.nn.BatchNorm2d):
            #     prune.remove(sub_module, 'weight')
            #     prune.remove(sub_module, 'bias')
            elif isinstance(sub_module, torch.nn.Module):
                self.remove_prune_model(sub_module)

    def prune_layer(self, model, step):
        for sub_idx, sub_module in model._modules.items():
            if isinstance(sub_module, torch.nn.Conv2d):
                # print(sub_idx, 'Conv2d')
                prune.l1_unstructured(sub_module, 'weight', amount=step)
            # elif isinstance(sub_module, torch.nn.Linear) or isinstance(sub_module, torch.nn.BatchNorm2d):
            #     # print(sub_idx, 'Linear')
            #     prune.l1_unstructured(sub_module, 'weight', amount=step)
            #     prune.l1_unstructured(sub_module, 'bias', amount=step)
            elif isinstance(sub_module, torch.nn.Module):
                # print(f"Entering layer[{sub_idx}]: {sub_module._get_name()}")
                self.prune_layer(sub_module, step)

    def prune_model(self, model, target_amount=0.3, threshold=1, step=0.1, max_iter=5, pass_normal=False, verbose=2):
        print("\nPruning Configs")
        print(f"- target_amount: {target_amount:.4f}")
        print(f"- loss_fn: {self.loss_fn}")
        print(f"- threshold: {threshold}")
        print(f"- step: {step}")
        print(f"- max_iter: {max_iter}")
        print(f"- pass_normal: {pass_normal}")

        if not pass_normal:
            print("\ntesting normal model...")
            normal_acc, normal_avg_loss = test(self.tuning_dataloader, model, loss_fn=self.loss_fn, verbose=verbose)
            print(f"normal model test result: acc({normal_acc:.4f}) avg_loss({normal_avg_loss:.4f})")
        else:
            print("normal model test passed")
            normal_acc = 100
            normal_avg_loss = 0

        chkpoint = model.state_dict()
        chkpoint_pruning_amount = 0
        chkpoint_acc = 0
        chkpoint_avg_loss = 0
        current_density = 1
        step_cnt = 0

        while True:
            step_cnt += 1
            print(f"iter: {step_cnt}")
            pruning_step_succeed = False
            current_density *= (1 - step)
            self.prune_layer(model, step)
            current_acc, current_avg_loss = 100, 0

            for _ in range(max_iter):
                train(self.tuning_dataloader, model, loss_fn=self.loss_fn, optimizer=self.optimizer, verbose=verbose)
                current_acc, current_avg_loss = test(self.tuning_dataloader, model, loss_fn=self.loss_fn, verbose=verbose)

                if current_acc > normal_acc - threshold:
                    pruning_step_succeed = True
                    break

            if not pruning_step_succeed:
                model.load_state_dict(chkpoint)
                print(f"pruning failed: acc({current_acc:.4f}) avg_loss({current_avg_loss:.4f})")
                print(f"pruning amount: {chkpoint_pruning_amount}")
                return model

            chkpoint = model.state_dict()
            chkpoint_pruning_amount = 1 - current_density
            chkpoint_acc = current_acc
            chkpoint_avg_loss = current_avg_loss
            print(f"check point generated: pamount({chkpoint_pruning_amount:.4f}) acc({chkpoint_acc:.4f}) "
                  f"avg_loss({chkpoint_avg_loss:.4f})\n")

            if round(chkpoint_pruning_amount, 1) >= target_amount:
                break

        model.load_state_dict(chkpoint)
        print(f"pruning succeed: acc({chkpoint_acc}) avg_loss({chkpoint_avg_loss})")
        print(f"pruning_amount: {chkpoint_pruning_amount}")
        return model