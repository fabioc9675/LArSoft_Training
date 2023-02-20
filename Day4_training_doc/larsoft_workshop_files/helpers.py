import copy, cv2, math, numpy as np, os, time, torch, torchvision as tv
from tqdm.notebook import tqdm

def set_seed(seed):
    """Set the various seeds and flags to ensure deterministic performance

        Args:
            seed: The random seed
    """
    torch.backends.cudnn.deterministic = True
    torch.backends.cudnn.benchmark = False
    np.random.seed(seed)
    torch.manual_seed(seed)
    torch.cuda.manual_seed(seed)


def count_classes(dataloader, num_classes):
    """Count the number of instances of each class in the training set

        Args:
            dataloader: The dataloader containing the training set
            num_classes: The number of classes expected in the training set

        Returns:
            A list of the number of instances of each class
    """
    count = np.zeros(num_classes)
    for batch in tqdm(dataloader, desc='Counting class frequency', miniters=100):
        _, truth = batch
        unique, counts = torch.unique(truth, return_counts=True)
        unique = [ u.item() for u in unique ]
        counts = [ c.item() for c in counts ]
        this_dict = dict(zip(unique, counts))
        for key in this_dict:
            count[key] += this_dict[key]
    return count


def get_class_weights(stats):
    """Get the weights for each class

        Each class has a weight inversely proportional to the number of instances in the training set

        Args:
            stats: The number of instances of each class

        Returns:
            The weights for each class
    """
    weights = 1. / stats
    return [weight / sum(weights) for weight in weights]