import torch
import torch.nn as nn
import numpy as np

class RBFKernelStd(nn.Module):
    def __init__(self):
        super(RBFKernelStd, self).__init__()
        self.coefficient = 1 / np.sqrt(2 * np.pi)
        
    def forward(self, x):
        return torch.exp(-0.5 * x * x) * self.coefficient

class RBF1D(nn.Module):
    def __init__(self, n_params=20):
        super(RBF1D, self).__init__()
        self.n_params = n_params
        self.kernel = RBFKernelStd()
        self.one = torch.Tensor([1.])
        self.a = nn.Parameter(torch.ones(self.n_params), requires_grad=True)
        self.b = nn.Parameter(torch.ones(self.n_params), requires_grad=True)
        self.linear = nn.Linear(n_params, 1, bias=True)
        #self.w = nn.Parameter(torch.ones(self.n_params + 1), requires_grad=True)
        self.init()
    
    def init(self):
        self.a.data.normal_(0, 0.2)
        self.b.data.normal_(0, 0.2)
        #self.w.data.normal_(0, 0.2)
        self.linear.weight.data.normal_(0, 0.2)
        
    def forward(self, x):
        g = self.kernel(self.a * x + self.b)
        #g_aug = torch.cat([g, self.one], dim=0)
        y = self.linear(g)#torch.dot(self.w, g_aug)
        return y
