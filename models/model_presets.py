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
    'SqueezeNet': ModelConfig(torchvision.models.squeezenet1_0, weights=torchvision.models.SqueezeNet1_0_Weights.IMAGENET1K_V1),
    'InceptionV3': ModelConfig(torchvision.models.inception_v3, weights=torchvision.models.Inception_V3_Weights.IMAGENET1K_V1),
}