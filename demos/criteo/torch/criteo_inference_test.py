import os
from absl.testing import absltest
import torch
from demos.criteo.torch.model import CriteoHELRM

SPLIT_1_SIZES = [1836] * 12 + [1841]
SPLIT_2_SIZES = [1836] * 12 + [1841]
vocab_sizes = SPLIT_1_SIZES + SPLIT_2_SIZES


class CriteoInferenceTest(absltest.TestCase):

  def test_inference_matches_expected(self):
    model_path = 'demos/criteo/data/criteohelrm.pth'
    sample_path = (
        'demos/criteo/data/sample.pt'
    )

    self.assertTrue(os.path.exists(model_path), f'{model_path} not found')
    self.assertTrue(os.path.exists(sample_path), f'{sample_path} not found')

    checkpoint = torch.load(model_path, map_location='cpu')
    state_dict = (
        checkpoint['weights']
        if isinstance(checkpoint, dict) and 'weights' in checkpoint
        else checkpoint
    )
    model = CriteoHELRM(vocab_sizes)
    state_dict = model.remap_orion_state_dict(state_dict)
    model.load_state_dict(state_dict)
    model.eval()

    sample = torch.load(sample_path, map_location='cpu')
    dense = sample['dense']
    sparse_x1 = sample['sparse_x1']
    sparse_x2 = sample['sparse_x2']
    labels = sample['labels']
    expected_predictions = sample.get('predictions', None)

    with torch.no_grad():
      predictions = model(dense, sparse_x1, sparse_x2)

    if expected_predictions is not None:
      torch.testing.assert_close(
          predictions, expected_predictions, rtol=1e-5, atol=1e-5
      )

    binary_predictions = (predictions >= 0.5).float()
    accuracy = (binary_predictions == labels).float().mean()
    print(f'\nModel accuracy on this sample: {accuracy.item():.4f}')
    self.assertGreater(accuracy.item(), 0.70)


if __name__ == '__main__':
  absltest.main()
