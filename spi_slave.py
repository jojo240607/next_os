# spi_slave.py
from renode_peripherals import *

class MySPISlave(Peripheral):
    def init(self):
        # 注册为 SPI 从设备，并定义回调函数
        self.spi_slave = self.CreateSPISlave()
        self.spi_slave.OnSend += self.OnSend
        self.spi_slave.OnReceive += self.OnReceive

    def OnSend(self, data):
        # 当从设备成功发送数据给主机后调用
        self.Log(f"SPI Slave sent: {data}")

    def OnReceive(self, data):
        # 当从设备收到主机发来的数据时调用
        self.Log(f"SPI Slave received: {data}")
        # 在这里，你可以根据收到的命令，通过 self.spi_slave.Send(data) 来回复数据