import copy, cv2, math, numpy as np, os, time, torch, torchvision as tv
import sklearn.metrics as skm

def update_statistics(phase_statistics, loss, predictions, truths):
    """Update the statistics dictionary

        Args:
            phase_statistics: The dictionary of statistics for a phase
            epoch: The epoch of training to which the statistics apply
            loss: The loss for the epoch
            predicitions: The network predictions for the epoch
            truths: The true classifications
    """
    preds_flat = torch.cat(predictions).cpu().numpy()
    truths_flat = torch.cat(truths).cpu().numpy()
    phase_statistics['loss'].append(loss)
    phase_statistics['accuracy'].append(skm.accuracy_score(truths_flat, preds_flat))
    phase_statistics['f1'].append(skm.f1_score(truths_flat, preds_flat, average='macro'))


def print_statistics(statistics, phase, epoch):
    """Print the statistics for a given phase and epoch

        Args:
            statistics: The dictionary of statistics
            phase: The phase of interest
            epoch: The epoch of training to which the statistics apply
    """
    print(f"=== {phase.title()} {epoch} ===")
    print('    Loss: {:.4f}'.format(statistics[phase]['loss'][-1]))
    print('    Accuracy: {:.4f}'.format(statistics[phase]['accuracy'][-1]))
    print('    F1: {:.4f}'.format(statistics[phase]['f1'][-1]))


def get_resnet_transforms():
    """Creates the training and validation set transformations for input images

        Returns:
            A dictionary containing the list for transforms to apply in training ('train') and validation ('val')
    """
    return {
        'train': tv.transforms.Compose(
            [tv.transforms.Resize([224, 224]),
                tv.transforms.RandomVerticalFlip(),
                tv.transforms.ToTensor(),
                tv.transforms.Normalize(mean=[0.485, 0.456, 0.406],
                                  std=[0.229, 0.224, 0.225])]),
        'val': tv.transforms.Compose(
            [tv.transforms.Resize([224, 224]),
                tv.transforms.ToTensor(),
                tv.transforms.Normalize(mean=[0.485, 0.456, 0.406],
                                  std=[0.229, 0.224, 0.225])])
    }