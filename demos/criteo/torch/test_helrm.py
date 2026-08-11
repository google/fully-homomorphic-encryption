import torch
from demos.criteo.torch import model

try:
    print("Trying to instantiate HELRM")
    # Criteo has 26 sparse fields.
    sparse_sizes = [1000] * 26
    helrm = model.HELRM(sparse_sizes, compress_threshold=20000, base=4)
    print("HELRM instantiated successfully")
    
    # Try to run forward with dummy input
    dense = torch.randn(1, 13)
    sparse = torch.randint(0, 1000, (1, 26))
    fhe_dense, fhe_sparse = helrm.fhe_input(dense, sparse)
    print("fhe_sparse shape:", fhe_sparse.shape)
    
    # This might fail if nn.Add doesn't exist or if nn.Embedding fails with float
    out = helrm(fhe_dense, fhe_sparse)
    print("Forward output shape:", out.shape)
    
except Exception as e:
    print("Failed:", e)
