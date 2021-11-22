# M031BSP_USCI_I2C_Monitor
 M031BSP_USCI_I2C_Monitor

update @ 2021/11/22

1. use USCI I2C to monitor I2C bus , UI2C0_SDA(PA.10), UI2C0_SCL(PA.11)

2. below is terminal log , when I2C master send format as below 

red box : 8 bit device address with Write (0xEC) , device register 0x40  / 0x 58

blue box : 8 bit device address with Write (0xEC) , device register 0x00

yellow box : 8 bit device address with Read (0xED) , READ data 3 bytes

![image](https://github.com/released/M031BSP_USCI_I2C_Monitor/blob/main/monitor_result.jpg)

