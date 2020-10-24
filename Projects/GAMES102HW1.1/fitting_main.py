import torch
import torch.nn as nn
import torch.optim as optim
from taichi import GUI
import rbf
import numpy as np

class GUI_Config:
    function_range = [-1, 1]#[-400, 400]
    canvas_size = [600, 600]
    background_color = 0x112F41
    line_segments = []
    points = []
    
    @staticmethod
    def normalized(data):
        a, b = GUI_Config.function_range[0], GUI_Config.function_range[1]
        
        return tuple((d - a) / (b - a) for d in data)
    
    @staticmethod
    def denormalized(data):
        a, b = GUI_Config.function_range[0], GUI_Config.function_range[1]
        return tuple(a * (1 - d) + b * d for d in data)
    
    @staticmethod
    def add_line_segments(rbf_training):
        GUI_Config.line_segments = []
        for x in range(0, GUI_Config.canvas_size[0] + 1, 1):
            xn = x / GUI_Config.canvas_size[0]
            xd = GUI_Config.denormalized((xn,))[0]
            yd = rbf_training.evaluate(xd)
            yn = GUI_Config.normalized((yd,))[0]
            GUI_Config.line_segments.append((xn, yn))
            
    
    @staticmethod
    def draw(gui):
        if len(GUI_Config.line_segments) >= 2:
            GUI_Config.line_segments.sort(key=lambda x: x[0])
            for i in range(0, len(GUI_Config.line_segments) - 1):
                gui.line(GUI_Config.line_segments[i], 
                         GUI_Config.line_segments[i + 1], 
                         4, 0xFFFFFF)
        for p in GUI_Config.points:
            gui.circle(p, 0xFF0000, 8)
    

class RBF_Training:
    def __init__(self, **kwargs):
        self.span = kwargs.get('span', 8)
        self.lr = kwargs.get('lr', 0.05)
        self.max_epoch = kwargs.get('max_epoch', 1000)
        self.epsilon = kwargs.get('epsilon', 1e-6)
        self.model = rbf.RBF1D(self.span)
        self.datas = {}
        self.optimizer = optim.Adam(self.model.parameters(), lr=self.lr)
        self.loss_fun = nn.MSELoss()
        self.last_loss = np.nan
        
    def update_model(self, **kwargs):
        self.span = kwargs.get('span', self.span)
        self.lr = kwargs.get('lr', self.lr)
        self.max_epoch = kwargs.get('max_epoch', self.max_epoch)
        self.epsilon = kwargs.get('epsilon', self.epsilon)
        self.model = rbf.RBF1D(self.span)
        self.optimizer = optim.Adam(self.model.parameters(), lr=self.lr)
        
    def train(self):
        self.model.init()
        self.model.train()
        for epoch in range(self.max_epoch):
            for x in self.datas:
                y = self.datas[x]
                xt = torch.Tensor([x])
                yt = torch.Tensor([y])
                
                self.optimizer.zero_grad()
                y_prediction = self.model(xt)
                loss = self.loss_fun(y_prediction, yt)
                loss.backward()
                self.optimizer.step()

            print("[Epoch: {:>4}] loss = {:>.9}".format(epoch + 1, loss.item()))
            if loss.item() < self.epsilon:
                break
        self.last_loss = loss.item()
        print(" [*] Training finished!")
    
    def evaluate(self, x):
        y = self.model(torch.Tensor([x])).item()
        return y
        

if __name__ == "__main__":
    esilon_times = 10000
    rbf_training = RBF_Training()
    gui = GUI("RBF Network Fitting (right click to add point)", 
              (GUI_Config.canvas_size[0], GUI_Config.canvas_size[1]), 
              background_color=GUI_Config.background_color)
    KEY_CLEAR = gui.button('clear')
    KEY_TRAIN = gui.button('train')
    lr_slide = gui.slider('learning rate', 1e-3, 1, step=1e-3)
    lr_slide.value = rbf_training.lr
    epsilon_slide = gui.slider('epsilon * %d'%esilon_times, 1e-3, 1, step=1e-3)
    epsilon_slide.value = rbf_training.epsilon * esilon_times
    max_epoch_slide = gui.slider('max epoch', 100, 5000, step=100)
    max_epoch_slide.value = rbf_training.max_epoch
    span_slide = gui.slider('span', 5, 50, step=1)
    span_slide.value = rbf_training.span
    
    while gui.running:
        event = None
        while gui.get_event():
            event = gui.event
            print(event.pos, event.key, event.type)
            break
        max_epoch_slide.value = int(max_epoch_slide.value)
        span_slide.value = int(span_slide.value)
        rbf_training.update_model(lr=lr_slide.value, 
                                  epsilon=epsilon_slide.value / esilon_times,
                                  max_epoch=int(max_epoch_slide.value),
                                  span=int(span_slide.value))
        if event:
            #print(event.pos, event.key, event.type)
            if event.key == GUI.RMB and event.type == GUI.RELEASE:
                print('Add point')
                mouse_x, mouse_y = gui.get_cursor_pos()
                GUI_Config.points.append((mouse_x, mouse_y))
                canvas_x, canvas_y = GUI_Config.denormalized((mouse_x, mouse_y))
                rbf_training.datas[canvas_x] = canvas_y
            
            if event.key == KEY_CLEAR or event.key == GUI.SPACE and event.type == GUI.RELEASE:
                print('Clear')
                rbf_training.datas = {}
                GUI_Config.points = []
                GUI_Config.line_segments = []
                
            if event.key == KEY_TRAIN:
                print('Train')
                if len(rbf_training.datas) >= 2:
                    GUI_Config.line_segments = []
                    GUI_Config.draw(gui)
                    gui.text('Last loss : %.9f'%rbf_training.last_loss, (0.01, 0.99), 24)
                    gui.show()
                    rbf_training.train()
                    GUI_Config.add_line_segments(rbf_training)
                    
        GUI_Config.draw(gui)
        gui.text('Last loss : %.9f'%rbf_training.last_loss, (0.01, 0.99), 24)
        gui.show()
        event = None
    gui.close()
        