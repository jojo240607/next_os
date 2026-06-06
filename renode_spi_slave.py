"""
Renode SPI 调试外设 —— 模拟一个带寄存器映射的 SPI 从设备

帧格式（每字节 MOSI 入 / MISO 出）：
  命令字节        地址高8位      地址低8位       数据字节...
  ─────────      ─────────      ─────────      ─────────
  0x03: 读      ADDR[15:8]     ADDR[7:0]      返回连续数据（地址自增）
  0x02: 写      ADDR[15:8]     ADDR[7:0]      数据字节... （地址自增）
  0x9F: 读ID     无关           无关            返回 3 字节 ID: 0xDE 0xAD 0xBE

内部寄存器空间: 256 字节
  0x00-0x03: 设备 ID (DE AD BE EF)
  0x04-0x07: 计数器（模拟传感器读数，每次读取后自增）
  0x10-0x1F: 用户可读写区（初始化为递增模式 0xA0-0xAF）
  其余: 0x00

用法（.resc 中）:
  machine LoadPlatformDescriptionFromString """
      spiDebug: Python.PythonPeripheral @ spi1 0x00
          size: 0x100
          script: @renode_spi_slave.py
  """
"""
import emulation


class SPIDebugSlave(object):
    """SPI 从设备仿真 —— 命令/地址/数据协议"""

    # ── 命令码 ──
    CMD_READ_ID = 0x9F  # 读设备 ID
    CMD_READ    = 0x03  # 读寄存器
    CMD_WRITE   = 0x02  # 写寄存器

    def __init__(self):
        self._regs = bytearray(256)
        self._init_registers()

        # 事务状态机
        self._state   = 0   # 0=等命令, 1=等地址高, 2=等地址低
        self._cmd     = 0
        self._addr    = 0
        self._rsp_idx = 0   # 读 ID 响应索引

    def _init_registers(self):
        """填充初始寄存器值"""
        # 设备 ID
        self._regs[0] = 0xDE
        self._regs[1] = 0xAD
        self._regs[2] = 0xBE
        self._regs[3] = 0xEF
        # 模拟传感器值
        self._regs[4] = 0x42
        self._regs[5] = 0x00
        self._regs[6] = 0x00
        self._regs[7] = 0x00
        # 用户可读写区
        for i in range(0x10, 0x20):
            self._regs[i] = 0xA0 + (i - 0x10)

    # ── SPI 回调 ──
    def handle_byte(self, mosibyte):
        """每收到一个 SPI 字节时调用，返回 MISO 字节。

        注意: 方法名由 Renode 框架约定，可能是:
          - OnSPIByte (Renode 旧版)
          - handle_byte / SPIReceiveByte (新版)
        请根据 Renode 版本调整方法名。
        """
        if self._state == 0:
            return self._handle_cmd(mosibyte)
        elif self._state == 1:
            return self._handle_addr_high(mosibyte)
        elif self._state == 2:
            return self._handle_addr_low(mosibyte)
        elif self._state == 3:
            return self._handle_data(mosibyte)
        else:
            return 0x00

    def _handle_cmd(self, b):
        self._cmd = b
        if b == self.CMD_READ_ID:
            self._rsp_idx = 0
            self._state = 0  # 保持状态，下一个字节直接读 ID
            return 0xFF      # dummy
        elif b == self.CMD_READ or b == self.CMD_WRITE:
            self._state = 1  # 等地址高
            return 0xFF      # dummy
        else:
            self._state = 0  # 未知命令，忽略
            return 0xFF

    def _handle_addr_high(self, b):
        self._addr = (b & 0xFF) << 8
        self._state = 2  # 等地址低
        return 0xFF  # dummy

    def _handle_addr_low(self, b):
        self._addr |= (b & 0xFF)
        if self._cmd == self.CMD_READ:
            self._state = 3  # 进入读数据模式
            val = self._regs[self._addr]
            self._addr = (self._addr + 1) & 0xFF
            return val       # 返回第一个数据字节
        else:
            self._state = 3  # 进入写数据模式
            return 0xFF      # dummy

    def _handle_data(self, b):
        if self._cmd == self.CMD_READ:
            val = self._regs[self._addr]
            self._addr = (self._addr + 1) & 0xFF
            return val
        else:
            # CMD_WRITE
            self._regs[self._addr] = b & 0xFF
            self._addr = (self._addr + 1) & 0xFF
            return 0xFF  # dummy

    # ── 读 ID 专用 ──
    def _handle_read_id(self):
        """返回设备 ID 序列"""
        ids = [0xDE, 0xAD, 0xBE]
        v = ids[self._rsp_idx] if self._rsp_idx < 3 else 0x00
        self._rsp_idx += 1
        return v


# ── 调试工具函数（可在 Renode 的 python 交互中调用） ──
# 例如在 Renode Monitor 中执行:
#   python "import renode_spi_slave as s; s.dump()"

def dump():
    """打印当前寄存器内容"""
    try:
        import renode
        spi = renode.get_peripheral('spiDebug')
        if spi:
            print("=== SPI Slave Register Dump ===")
            for i in range(0, 256, 16):
                hexline = ' '.join(f'{spi._regs[i+j]:02X}' for j in range(16))
                ascii_line = ''.join(chr(spi._regs[i+j]) if 32 <= spi._regs[i+j] < 127 else '.' for j in range(16))
                print(f"  {i:02X}: {hexline}  {ascii_line}")
    except Exception as e:
        print(f"Dump error: {e}")
