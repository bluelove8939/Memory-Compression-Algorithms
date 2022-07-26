import torchvision


class ModelConfig(object):
    def __init__(self, model_type, weights=None):
        self.model_type = model_type
        self.weights = weights

    def generate(self):
        return self.model_type(weights=self.weights)


imagenet_pretrained = {
    'ResNet50': ModelConfig(torchvision.models.resnet50, weights=torchvision.models.ResNet50_Weights.IMAGENET1K_V1),
    'AlexNet': ModelConfig(torchvision.models.alexnet, weights=torchvision.models.AlexNet_Weights.IMAGENET1K_V1),
    'VGG16': ModelConfig(torchvision.models.vgg16, weights=torchvision.models.VGG16_Weights.IMAGENET1K_V1),
}