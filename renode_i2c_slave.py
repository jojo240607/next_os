"""
Renode I2C 调试外设 —— 模拟一个 I2C EEPROM / 传感器设备

协议:
  写操作: START → 设备地址+W → 寄存器地址 → 数据字节... → STOP
  读操作: START → 设备地址+W → 寄存器地址 → RESTART → 设备地址+R → 数据字节... → STOP
         (或: START → 设备地址+R → 从当前地址读取)

I2C 从地址: 0x50（可通过 resc 文件自定义）

内部空间: 256 字节
  0x00-0x07: 设备信息 (序列号/版本)
  0x08-0x0F: 传感器模拟值（温度/湿度/光照）
  0x10-0x2F: 用户数据区（初始化为 0x00）
  0x30-0x3F: 预填充测试数据 (0xB0-0xBF)

用法（.resc 中）:
  machine LoadPlatformDescriptionFromString """
      i2cDebug: Python.PythonPeripheral @ i2c1
          address: 0x50
          script: @renode_i2c_slave.py
  """
"""
import emulation


class I2CDebugSlave(object):
    """I2C 从设备仿真 —— 通用寄存器读写"""

    SLAVE_ADDR = 0x50  # 7 位从地址

    def __init__(self):
        self._regs = bytearray(256)
        self._init_registers()

        # I2C 事务状态
        self._reg_ptr   = 0   # 当前寄存器指针
        self._is_write  = True
        self._addr_matched = False

    def _init_registers(self):
        """填充初始寄存器值"""
        # 设备信息
        self._regs[0x00] = 0x12  # 型号高字节
        self._regs[0x01] = 0x34  # 型号低字节
        self._regs[0x02] = 0x01  # 版本号
        self._regs[0x03] = 0x00  # 保留
        # 传感器模拟值
        self._regs[0x08] = 0x1A  # 温度高(约 26°C)
        self._regs[0x09] = 0x80  # 温度低(小数)
        self._regs[0x0A] = 0x35  # 湿度高(约 53%)
        self._regs[0x0B] = 0x00  # 湿度低
        self._regs[0x0C] = 0x01  # 光照高
        self._regs[0x0D] = 0xF4  # 光照低(500 lux)
        # 测试数据区
        for i in range(0x30, 0x40):
            self._regs[i] = 0xB0 + (i - 0x30)

    # ── I2C 回调 ──
    def handle_write(self, data):
        """I2C 主设备写入一个字节。

        第一个字节: 寄存器地址指针
        后续字节: 写入寄存器，地址自增
        """
        self._regs[self._reg_ptr] = data & 0xFF
        self._reg_ptr = (self._reg_ptr + 1) & 0xFF

    def handle_read(self):
        """I2C 主设备读取一个字节。返回当前寄存器值，地址自增。"""
        val = self._regs[self._reg_ptr]
        self._reg_ptr = (self._reg_ptr + 1) & 0xFF
        return val

    def handle_write_address(self, addr, is_read):
        """I2C 地址匹配时的回调。

        Args:
            addr: 7 位设备地址
            is_read: True 表示主机请求读, False 表示写
        Returns:
            True 表示 ACK（响应）, False 表示 NACK
        """
        if addr == self.SLAVE_ADDR:
            self._addr_matched = True
            if is_read:
                # 读操作: 之前必须先写过寄存器地址
                return True
            else:
                # 写操作: 最多 1 字节即可 set 寄存器指针
                return True
        return False

    def handle_start(self):
        """I2C START 条件"""
        pass

    def handle_stop(self):
        """I2C STOP 条件 —— 事务结束"""
        self._addr_matched = False

    def handle_write_to_pointer(self, data):
        """写寄存器地址指针（收到第一个数据字节时）。

        注意: 某些 Renode 版本将此回调命名为 OnI2CByte
        第一个收到的主机写字节用于设置寄存器指针。
        """
        self._reg_ptr = data & 0xFF

    def handle_read_from_pointer(self):
        """从当前指针读取一个字节"""
        val = self._regs[self._reg_ptr]
        self._reg_ptr = (self._reg_ptr + 1) & 0xFF
        return val


# ── 调试工具函数 ──
def dump():
    """在 Renode Monitor 的 python 交互中打印寄存器内容"""
    try:
        import renode
        dev = renode.get_peripheral('i2cDebug')
        if dev:
            print("=== I2C Slave Register Dump (0x50) ===")
            print("  [Device Info]")
            print(f"    Model:     {dev._regs[0]:02X}{dev._regs[1]:02X}")
            print(f"    Version:   {dev._regs[2]}")
            print("  [Sensor Values]")
            temp = dev._regs[8] + dev._regs[9] / 256.0
            hum  = dev._regs[0x0A] + dev._regs[0x0B] / 256.0
            lux  = (dev._regs[0x0C] << 8) | dev._regs[0x0D]
            print(f"    Temp:      {temp:.1f} C")
            print(f"    Humidity:  {hum:.1f} %")
            print(f"    Light:     {lux} lux")
            print("  [Test Area 0x30-0x3F]")
            hexline = ' '.join(f'{dev._regs[0x30+i]:02X}' for i in range(16))
            print(f"    {hexline}")
    except Exception as e:
        print(f"Dump error: {e}")
