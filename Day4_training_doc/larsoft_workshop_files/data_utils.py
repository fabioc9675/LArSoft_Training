import copy, cv2, math, numpy as np, os, time, torch, torchvision as tv
from helpers import *

def make_datasets(directory, transform=None, valid_size=10000, train_size=None):
    """Construct training and validation sets from images in a specified top-level directory

        Args:
            directory: Top-level directory containing images (sub-directories should correspond to the classes [0, 1, 2])
            transform: A dictionary containing the training and validation transforms, keyed as 'train'/'val'
            valid_size: The number of samples to use in the validation set
        Returns:
            A tuple containing the training and validation sets
    """
    labels = []
    filenames = []
    for path in os.scandir(directory):
        if path.is_dir():
            label = int(path.name)
            if not label in [0, 1, 2, 3, 4, 5]:
                continue

            if label in [0, 1]:
                meta_label = 0
            elif label in [2, 3]:
                meta_label = 1
            elif label == 4:
                meta_label = 2
            else:
                meta_label = 3
            for filename in os.scandir(path):
                if filename.is_file():
                    filenames.append(filename.path)
                    labels.append(meta_label)
    labels = np.array(labels)
    filenames = np.array(filenames)
    sample = np.random.permutation(len(labels))
    train_sample = sample[valid_size:] if not train_size else \
        sample[valid_size:valid_size + train_size]
    valid_sample = sample[:valid_size]
    train_transform = transform['train'] if transform else None
    valid_transform = transform['val'] if transform else None
    train_set = NeutrinoDataset(labels[train_sample], filenames[train_sample], transform=train_transform)
    valid_set = NeutrinoDataset(labels[valid_sample], filenames[valid_sample], transform=valid_transform)

    return train_set, valid_set


def prepare_dataloaders(training_set, validation_set, batch_size, num_classes):
    """Create and prepares dataloaders for the training loops.

        This function also determines and prints the frequency for the classes in the training and validation set

        Args:
            training_set: The NeutrinoDataset for training
            validation_set: The NeutrinoDataset for validation
            batch_size: The number of images in a batch

        Returns:
            A tuple containing a ditionary of the training ('train') and validation ('val') dataloaders, a
            corresponding dictionary noting the size of the respective datasets and the training set class weights
    """
    from torch.utils.data import DataLoader

    training_loader = DataLoader(training_set, batch_size=batch_size, shuffle=True, num_workers=0, drop_last=False)
    validation_loader = DataLoader(validation_set, batch_size=batch_size, shuffle=False, num_workers=0, drop_last=False)

    training_counts = count_classes(training_set, num_classes)
    training_weights = get_class_weights(training_counts)

    print(f"Counts (Training): {training_counts}")
    print(f"Weights (Training): {training_weights}")

    validation_counts = count_classes(validation_set, num_classes)
    validation_weights = get_class_weights(validation_counts)

    print(f"Counts (Validation): {validation_counts}")
    print(f"Weights (Validation): {validation_weights}")

    dataloaders = {'train': training_loader, 'val': validation_loader}
    dataset_sizes= {'train': len(training_set), 'val': len(validation_set)}

    return dataloaders, dataset_sizes, training_weights


from torch.utils.data import Dataset
class NeutrinoDataset(Dataset):
    def __init__(self, labels, filenames, transform=None):
        self.labels = labels
        self.filenames = filenames
        self.transform = transform

    def __len__(self):
        return len(self.filenames)

    def __getitem__(self, idx):
        image = self.open_image(self.filenames[idx])
        label = torch.as_tensor(self.labels[idx], dtype=torch.long)
        if self.transform:
            image = self.transform(image)
        return image, label


    def open_image(self, path):
        """Retrieve an image.

            Args:
                path: The path of the image

            Returns:
                The image
        """
        from PIL import Image
        img = Image.open(path)
        if img.mode  != 'RGB':
            img = img.convert('RGB')
        return img