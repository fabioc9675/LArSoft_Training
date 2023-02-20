import copy, cv2, math, numpy as np, os, time, torch, torchvision as tv

def reinit_conv_layers(model, leak = 0.0, use_kaiming_normal=True):
    """Reinitialises convolutional layer weights.

        The default Kaiming initialisation in PyTorch is not optimal, this method
        reinitialises the layers using better parameters

        Args:
            model: The model whose convolutional layers are to be reinitialised.
            leak: The leakiness of ReLU (default: 0.0)
            use_kaiming_normal: Use Kaiming normal if True, Kaiming uniform otherwise (default: True)
    """
    for layer in model.modules():
        if isinstance(layer, torch.nn.Conv2d):
            if use_kaiming_normal:
                torch.nn.init.kaiming_normal_(layer.weight, a = leak)
            else:
                torch.nn.init.kaiming_uniform_(layer.weight, a = leak)
                layer.bias.data.zero_()


def save_model(model, filename):
    """Save the model

        Args:
            model: The model to save
            filename: The output filename, without file extension
    """
    torch.save(model.state_dict(), f"{filename}.pt")


def print_parameters(model):
    """Print the parameters of the model which are to be learned.

        Args:
            model: The model being trained
    """
    print("Parameters to learn:")
    for name, param in model.named_parameters():
        if param.requires_grad == True:
            print("\t",name)