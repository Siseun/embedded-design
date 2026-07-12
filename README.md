## 系统功能实现

### 1. 智能手表硬件平台搭建
完成智能手表硬件电路设计，包括 STM32 主控、电源模块、OLED 显示模块以及 MPU6050 传感器模块，并基于 FreeRTOS 实现多任务管理。
<img width="1706" height="1279" alt="1" src="https://github.com/user-attachments/assets/d228103e-7f64-4479-911a-7371a6616317" />

### 2. 时间显示功能
实现当前时间、日期信息的实时显示，并通过 OLED 屏幕进行动态刷新。
<img width="1706" height="1279" alt="2" src="https://github.com/user-attachments/assets/11bac005-af78-4007-86be-bf6c56db4dfc" /><img width="1706" height="1279" alt="4" src="https://github.com/user-attachments/assets/8a950eb2-d671-477e-9e6a-0078256b90e2" />
<img width="1706" height="1279" alt="3" src="https://github.com/user-attachments/assets/8ebd342e-88e2-4b57-b863-ef4e4d216179" />

### 3. 传感器数据显示功能
实现 MPU6050 传感器数据采集与显示，可实时查看加速度和角速度信息。
<img width="1706" height="1279" alt="6" src="https://github.com/user-attachments/assets/9ba787d0-08a9-432f-b671-92d1bd49da3c" />
<img width="1706" height="1279" alt="5" src="https://github.com/user-attachments/assets/9c9962a9-0c37-4915-a804-0d101e5ee3b2" />

### 4. 运动计步功能
基于加速度传感器实现运动计步算法，完成步数统计与运动数据记录功能，并在 OLED 界面进行显示。
<img width="1706" height="1279" alt="7" src="https://github.com/user-attachments/assets/76b99602-1642-44fa-91b4-6d0bcbb6b88c" />

### 5. 菜单切换功能
增加旋转编码器，实现多级菜单界面的切换、选择以及功能页面跳转。

### 6. 蓝牙通信功能（创新点）
增加蓝牙通信模块，实现智能手表与手机之间的连接与数据交互，并支持时间、日期及环境温度等信息的实时同步与显示。
