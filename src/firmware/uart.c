/*
 * MSP430G2553
 * 命令码：0x10(停止)、0x11(启动电压)、0x12(启动温度)、0x13(启动双通道)
 * 事件码：(0x01~0x09/0xFF)、错误码(0x00~0x0A)
 */
#include <msp430g2553.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// ====================== 通信协议核心宏定义 ======================
// 帧头/帧尾
#define FRAME_HEADER1   0xAA
#define FRAME_HEADER2   0x55
#define FRAME_TAIL      0x0D

// 帧类型
#define FRAME_TYPE_VOLTAGE   0x01  // 电压数据帧
#define FRAME_TYPE_TEMP      0x02  // 温度数据帧
#define FRAME_TYPE_DUAL      0x03  // 双通道数据帧
#define FRAME_TYPE_EVENT     0x04  // 事件上报帧
#define FRAME_TYPE_ACK       0x0F  // 应答帧

// 命令码（0x10~0x13）
#define CMD_STOP           0x10  // 停止所有采集
#define CMD_START_VOLTAGE  0x11  // 启动电压采集
#define CMD_START_TEMP     0x12  // 启动温度采集
#define CMD_START_AUTO     0x13  // 启动双通道采集

// 错误码（0x00~0x0A）
typedef enum {
    ERR_SUCCESS = 0x00,         // 成功
    ERR_INVALID_CMD = 0x01,     // 无效命令
    ERR_PARAM_ERROR = 0x02,     // 参数错误
    ERR_DEVICE_BUSY = 0x03,     // 设备忙
    ERR_HARDWARE_ERROR = 0x04,  // 硬件故障
    ERR_CRC_ERROR = 0x05,       // CRC校验失败
    ERR_TIMEOUT = 0x06,         // 操作超时
    ERR_NOT_CALIBRATED = 0x07,  // 未校准（预留）
    ERR_FLASH_ERROR = 0x08,     // Flash操作失败（预留）
    ERR_INVALID_STATE = 0x09,   // 状态无效
    ERR_OVERFLOW = 0x0A         // 数据溢出
} ErrorCode;

// 完整事件码（0x01~0x09、0xFF）
typedef enum {
    EVENT_VOLTAGE_HIGH = 0x01,    // 电压超过上限
    EVENT_VOLTAGE_LOW = 0x02,     // 电压低于下限
    EVENT_TEMP_HIGH = 0x03,       // 温度超过上限
    EVENT_TEMP_LOW = 0x04,        // 温度低于下限
    EVENT_BAT_LOW = 0x05,         // 电池电量低
    EVENT_BUF_OVERFLOW = 0x06,    // 数据缓冲区溢出
    EVENT_CALIB_DONE = 0x07,      // 校准完成（预留）
    EVENT_CFG_SAVED = 0x08,       // 配置已保存（预留）
    EVENT_RESET_DONE = 0x09,      // 设备复位完成（预留）
    EVENT_UNKNOWN_ERR = 0xFF      // 未知错误
} EventCode;

// 设备状态机
typedef enum {
    STATE_IDLE = 0,     // 空闲
    STATE_VOLTAGE = 1,  // 电压采集中
    STATE_TEMP = 2,     // 温度采集中
    STATE_AUTO = 3      // 双通道采集中
} DeviceState;

// ====================== 硬件适配宏定义 ======================
#define RX_BUF_SIZE 16          // 接收缓冲区
#define ADC_CHANNEL_VOLT 0      // P1.0 = 电压采集AD通道
#define ADC_CHANNEL_TEMP 3      // P1.3 = 温度采集AD通道
#define ADC_CHANNEL_BAT 4       // P1.4 = 电池电压检测AD通道
#define REF_VOLTAGE 1500        // ADC参考电压1500mV
#define PERIOD_UNIT 5
// 阈值配置
#define VOLT_HIGH_THRESHOLD 3000  // 电压上限3000mV（3.0V）
#define VOLT_LOW_THRESHOLD  500   // 电压下限500mV（0.5V）
#define TEMP_HIGH_THRESHOLD 6000  // 温度上限60.00℃
#define TEMP_LOW_THRESHOLD  0     // 温度下限0.00℃
#define BAT_LOW_THRESHOLD   2800  // 电池低电量阈值2800mV（2.8V）

// ====================== 全局变量 ======================
uint8_t rx_buf[RX_BUF_SIZE];    // 接收PC命令帧缓冲区
uint8_t rx_len = 0;             // 接收字节计数
DeviceState currentState = STATE_IDLE;  // 当前设备状态
uint8_t frame_seq = 0;          // 帧序号（自增，发送PC）
uint16_t volt_period = 50;      // 默认电压采集周期（ms）
uint16_t temp_period = 50;      // 默认温度采集周期（ms）
uint16_t buf_overflow_cnt = 0;  // 缓冲区溢出计数

// ====================== 帧结构体  =========================
// 命令帧
typedef struct {
    uint8_t header[2];      // 0-1：AA 55（所有帧通用）
    uint8_t length;         // 2：帧长度（7/8/9）
    uint8_t cmd;            // 3：命令码（0x10~0x13）
    uint8_t seq;            // 4：序列码（所有帧通用）
} CmdFrame;

// 应答帧
typedef struct {
    uint8_t header[2];      // 0xAA 0x55
    uint8_t length;         // 固定0x08
    uint8_t frame_type;     // 固定0x0F
    uint8_t seq;            // 对应PC命令帧的序号
    uint8_t err_code;       // 错误码（0x00~0x0A）
    uint8_t crc;            // CRC8校验
    uint8_t tail;           // 0x0D
} AckFrame;

// 电压数据帧
#pragma pack(1)
typedef struct {
    uint8_t header[2];      // 0xAA 0x55
    uint8_t length;         // 固定0x09
    uint8_t frame_type;     // 固定0x01
    uint8_t seq;            // 数据帧序号
    uint16_t voltage;       // 电压值（mV，小端模式）
    uint8_t crc;            // CRC8校验
    uint8_t tail;           // 0x0D
} VoltDataFrame;
#pragma pack()

// 温度数据帧
#pragma pack(1)
typedef struct {
    uint8_t header[2];      // 0xAA 0x55
    uint8_t length;         // 固定0x09
    uint8_t frame_type;     // 固定0x02
    uint8_t seq;            // 数据帧序号
    int16_t temp;           // 温度值（0.01℃，小端模式）
    uint8_t crc;            // CRC8校验
    uint8_t tail;           // 0x0D
} TempDataFrame;
#pragma pack()

// 双通道数据帧
#pragma pack(1)
typedef struct {
    uint8_t header[2];      // 0xAA 0x55
    uint8_t length;         // 固定0x0B
    uint8_t frame_type;     // 固定0x03
    uint8_t seq;            // 数据帧序号
    uint16_t voltage;       // 电压（mV）
    int16_t temp;           // 温度（0.01℃）
    uint8_t crc;            // CRC8校验
    uint8_t tail;           // 0x0D
} DualDataFrame;
#pragma pack()

// 事件上报帧
typedef struct {
    uint8_t header[2];      // 0xAA 0x55
    uint8_t length;         // 固定0x08
    uint8_t frame_type;     // 固定0x04
    uint8_t seq;            // 帧序号（自增）
    uint8_t event_code;     // 事件码（0x01~0x09、0xFF）
    uint8_t crc;            // CRC8校验
    uint8_t tail;           // 0x0D
} EventFrame;

// ====================== 核心工具函数 ======================

//CRC8-CCITT校验函数（多项式0x07，初始值0x00）
uint8_t crc8_ccitt(const uint8_t *data, uint16_t len) {
    uint8_t crc = 0x00;
    const uint8_t poly = 0x07;
    uint16_t i;
    uint8_t j;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = ((crc << 1) ^ poly) & 0xFF;
            } else {
                crc = (crc << 1) & 0xFF;
            }
        }
    }
    return crc;
}

// 自定义16位无符号数据字节交换
uint16_t swap_uint16(uint16_t value) {
    return (value >> 8) | (value << 8);
}

// 自定义16位有符号数据字节交换
int16_t swap_int16(int16_t value) {
    uint16_t u_val = (uint16_t)value;
    u_val = (u_val >> 8) | (u_val << 8);
    return (int16_t)u_val;
}

// 串口发送1字节
void UART_SendByte(uint8_t byte) {
    while (!(IFG2 & UCA0TXIFG));  // 等待发送缓冲区空
    UCA0TXBUF = byte;
}

//发送整帧数据
void SendFrame(uint8_t *frame, uint8_t len) {
    // 保存当前状态，退出低功耗
    uint8_t sr = __get_SR_register();
    __bic_SR_register(LPM0_bits);

    // 逐字节发送，等待每个字节发送完成
    uint8_t i = 0;
    for ( i = 0; i < len; i++) {
        while (!(IFG2 & UCA0TXIFG)); // 等待发送缓冲区空
        UCA0TXBUF = frame[i];
        while (!(IFG2 & UCA0TXIFG)); // 等待发送完成
    }

    // 恢复低功耗状态
    __bis_SR_register(sr);
}


// ====================== 应答帧处理 ======================

//打包并发送应答帧
void PackAndSendAckFrame(uint8_t cmd_seq, ErrorCode err) {
    AckFrame ack;

    ack.header[0] = FRAME_HEADER1;
    ack.header[1] = FRAME_HEADER2;
    ack.length = 0x08;          // 应答帧长度固定0x08
    ack.frame_type = FRAME_TYPE_ACK;
    ack.seq = cmd_seq;          // 对应PC命令的序号
    ack.err_code = err;
    ack.tail = FRAME_TAIL;

    uint8_t crc_data[4] = {ack.length, ack.frame_type, ack.seq, ack.err_code};
    ack.crc = crc8_ccitt(crc_data, 4);

    SendFrame((uint8_t *)&ack, 8);
}

// ====================== 事件上报帧处理  ========================

//打包并发送事件上报帧
void PackAndSendEventFrame(EventCode event) {
    EventFrame event_frame;

    event_frame.header[0] = FRAME_HEADER1;
    event_frame.header[1] = FRAME_HEADER2;
    event_frame.length = 0x08;       // 事件帧长度固定0x08
    event_frame.frame_type = FRAME_TYPE_EVENT;
    event_frame.seq = frame_seq++;   // 帧序号自增（和数据帧共用）
    event_frame.event_code = event;
    event_frame.tail = FRAME_TAIL;

    uint8_t crc_data[4] = {
        event_frame.length, event_frame.frame_type, event_frame.seq, event_frame.event_code
    };
    event_frame.crc = crc8_ccitt(crc_data, 4);

    SendFrame((uint8_t *)&event_frame, 8);
}

// 检测电池电量（P1.4）
uint16_t ReadBatteryVoltage(void) {
    uint16_t ad_value = 0;
    ADC10CTL0 = SREF_1 + ADC10SHT_2 + ADC10ON;
    ADC10CTL1 = INCH_4;                          // 选择AD通道4（P1.4）
    ADC10AE0 |= BIT4;                            // 使能P1.4模拟输入
    ADC10CTL0 |= ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);
    ad_value = ADC10MEM;
    ADC10CTL0 &= ~ADC10ON;
    return (uint16_t)(((uint32_t)ad_value * REF_VOLTAGE * 2) / 1024); // 分压补偿
}

// 检测各类阈值/异常（触发完整事件码上报）
void CheckAllThresholdAndReport(uint16_t volt, int16_t temp) {
    //  电压阈值检测
    if (volt > VOLT_HIGH_THRESHOLD) {
        PackAndSendEventFrame(EVENT_VOLTAGE_HIGH);
        DelayMs(10);
    } else if (volt < VOLT_LOW_THRESHOLD) {
        PackAndSendEventFrame(EVENT_VOLTAGE_LOW);
        DelayMs(10);
    }

    //  温度阈值检测
    if (temp > TEMP_HIGH_THRESHOLD) {
        PackAndSendEventFrame(EVENT_TEMP_HIGH);
        DelayMs(10);
    } else if (temp < TEMP_LOW_THRESHOLD) {
        PackAndSendEventFrame(EVENT_TEMP_LOW);
        DelayMs(10);
    }

    //  电池低电量检测
    uint16_t bat_volt = ReadBatteryVoltage();
    if (bat_volt < BAT_LOW_THRESHOLD) {
        PackAndSendEventFrame(EVENT_BAT_LOW);
        DelayMs(10);
    }

    //  缓冲区溢出检测
    if (buf_overflow_cnt > 0) {
        PackAndSendEventFrame(EVENT_BUF_OVERFLOW);
        buf_overflow_cnt = 0; // 清零计数
        DelayMs(10);
    }
}


// ====================== 数据帧处理 ======================

// 测试用   ADC采集电压值（P1.0）
uint16_t ReadVoltage(void) {
    uint16_t ad_value = 0;
    ADC10CTL0 = SREF_1 + ADC10SHT_2 + ADC10ON;
    ADC10CTL1 = INCH_0;
    ADC10AE0 |= BIT0;
    ADC10CTL0 |= ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);
    ad_value = ADC10MEM;
    ADC10CTL0 &= ~ADC10ON;
    return (uint16_t)(((uint32_t)ad_value * REF_VOLTAGE) / 1024);
}

//测试用   采集温度值（P1.3模拟输入）
int16_t ReadTemperature(void) {
    uint16_t ad_value = 0;
    ADC10CTL0 = SREF_1 + ADC10SHT_2 + ADC10ON;
    ADC10CTL1 = INCH_3;
    ADC10AE0 |= BIT3;
    ADC10CTL0 |= ADC10SC;
    while (ADC10CTL1 & ADC10BUSY);
    ad_value = ADC10MEM;
    ADC10CTL0 &= ~ADC10ON;
    return (int16_t)(((ad_value - 512) * 100) / 10);
}

// 打包并发送电压数据帧
void PackAndSendVoltFrame(void) {
    VoltDataFrame volt_frame;
    uint16_t volt = ReadVoltage();

    CheckAllThresholdAndReport(volt, 0);

    memset(&volt_frame, 0, sizeof(VoltDataFrame));

    volt_frame.header[0] = FRAME_HEADER1;
    volt_frame.header[1] = FRAME_HEADER2;
    volt_frame.length = 0x09;
    volt_frame.frame_type = FRAME_TYPE_VOLTAGE;
    volt_frame.seq = frame_seq++;
    volt_frame.voltage = swap_uint16(volt);
    volt_frame.tail = FRAME_TAIL;

    uint8_t crc_data[5] = {
        volt_frame.length, volt_frame.frame_type, volt_frame.seq,
        (uint8_t)(volt_frame.voltage & 0xFF), (uint8_t)(volt_frame.voltage >> 8)
    };
    volt_frame.crc = crc8_ccitt(crc_data, 5);

    if (sizeof(VoltDataFrame) != 9) {
        UART_SendByte(0xFE);
        UART_SendByte(sizeof(VoltDataFrame));
        UART_SendByte(0xFF);
    }

    SendFrame((uint8_t *)&volt_frame, sizeof(VoltDataFrame));
}
// 打包并发送温度数据帧
void PackAndSendTempFrame(void) {
    TempDataFrame temp_frame;
    int16_t temp = ReadTemperature();

    CheckAllThresholdAndReport(0, temp);

    memset(&temp_frame, 0, sizeof(TempDataFrame));
    temp_frame.header[0] = FRAME_HEADER1;
    temp_frame.header[1] = FRAME_HEADER2;
    temp_frame.length = 0x09;
    temp_frame.frame_type = FRAME_TYPE_TEMP;
    temp_frame.seq = frame_seq++;
    temp_frame.temp = swap_int16(temp);
    temp_frame.tail = FRAME_TAIL;

    uint8_t crc_data[5] = {
        temp_frame.length,
        temp_frame.frame_type,
        temp_frame.seq,
        (uint8_t)(temp_frame.temp & 0xFF),
        (uint8_t)(temp_frame.temp >> 8)
    };
    temp_frame.crc = crc8_ccitt(crc_data, 5);

    if (sizeof(TempDataFrame) != 9) {
        UART_SendByte(0xFE);
        UART_SendByte(sizeof(TempDataFrame));
        UART_SendByte(0xFF);
    }

    SendFrame((uint8_t *)&temp_frame, sizeof(TempDataFrame));
}

// 打包并发送双通道数据帧
void PackAndSendDualFrame(void) {
    DualDataFrame dual_frame;
    uint16_t volt = ReadVoltage();
    int16_t temp = ReadTemperature();

    CheckAllThresholdAndReport(volt, temp);

    memset(&dual_frame, 0, sizeof(DualDataFrame));

    dual_frame.header[0] = FRAME_HEADER1;
    dual_frame.header[1] = FRAME_HEADER2;
    dual_frame.length = 0x0B;
    dual_frame.frame_type = FRAME_TYPE_DUAL;
    dual_frame.seq = frame_seq++;
    dual_frame.voltage = swap_uint16(volt);
    dual_frame.temp = swap_int16(temp);
    dual_frame.tail = FRAME_TAIL;

    uint8_t crc_data[7] = {
        dual_frame.length, dual_frame.frame_type, dual_frame.seq,
        (uint8_t)(dual_frame.voltage & 0xFF), (uint8_t)(dual_frame.voltage >> 8),
        (uint8_t)(dual_frame.temp & 0xFF), (uint8_t)(dual_frame.temp >> 8)
    };
    dual_frame.crc = crc8_ccitt(crc_data, 7);

    if (sizeof(DualDataFrame) != 11) {
        UART_SendByte(0xFE);
        UART_SendByte(sizeof(DualDataFrame));
        UART_SendByte(0xFF);
    }

    SendFrame((uint8_t *)&dual_frame, sizeof(DualDataFrame));
}
// ====================== 命令帧处理  ===========================
// 解析命令帧
ErrorCode ParseCmdFrame(void) {
    CmdFrame *cmd_common = (CmdFrame *)rx_buf;
    ErrorCode err = ERR_SUCCESS;
    uint8_t send_crc = 0;
    uint8_t calc_crc = 0;

    // 1. 帧头校验
    if (cmd_common->header[0] != FRAME_HEADER1 || cmd_common->header[1] != FRAME_HEADER2) {
        return ERR_INVALID_CMD;
    }

    // 2. 根据帧长度处理CRC+参数
    switch (cmd_common->length) {
        case 0x07: // STOP命令
        {
            send_crc = rx_buf[5];
            // 校验范围：length+cmd+seq
            uint8_t crc_data[3] = {cmd_common->length, cmd_common->cmd, cmd_common->seq};
            calc_crc = crc8_ccitt(crc_data, 3);
            break;
        }
        case 0x08: // 电压/温度启动
        {
            uint8_t param = rx_buf[5];
            send_crc = rx_buf[6];
            // 校验范围：length+cmd+seq+param
            uint8_t crc_data[4] = {cmd_common->length, cmd_common->cmd, cmd_common->seq, param};
            calc_crc = crc8_ccitt(crc_data, 4);
            break;
        }
        case 0x09: // 双通道启动
        {
            uint8_t v_param = rx_buf[5];
            uint8_t t_param = rx_buf[6];
            send_crc = rx_buf[7];
            // 校验范围：length+cmd+seq+v_param+t_param
            uint8_t crc_data[5] = {cmd_common->length, cmd_common->cmd, cmd_common->seq, v_param, t_param};
            calc_crc = crc8_ccitt(crc_data, 5);
            break;
        }
        default:
            return ERR_PARAM_ERROR;
    }

    // 3. CRC通用校验
    if (calc_crc != send_crc) {
        return ERR_CRC_ERROR;
    }

    // 4. 设备忙检测
    if (currentState != STATE_IDLE && cmd_common->cmd != CMD_STOP) {
        return ERR_DEVICE_BUSY;
    }

    // 5. 解析命令+参数
    switch (cmd_common->cmd) {
        case CMD_STOP: // 0x10
            currentState = STATE_IDLE;
            break;

        case CMD_START_VOLTAGE: // 0x11
        {
            uint8_t volt_period = rx_buf[5];
            if (volt_period < 1 || volt_period > 255) {
                return ERR_PARAM_ERROR;
            }
            volt_period = volt_period * PERIOD_UNIT;
            if (volt_period < 5) {
                buf_overflow_cnt++;
                return ERR_OVERFLOW;
            }
            currentState = STATE_VOLTAGE;
            break;
        }

        case CMD_START_TEMP: // 0x12
        {
            uint8_t temp_period = rx_buf[5];
            if (temp_period < 1 || temp_period > 255) {
                return ERR_PARAM_ERROR;
            }
            temp_period = temp_period * PERIOD_UNIT;
            if (temp_period < 5) {
                buf_overflow_cnt++;
                return ERR_OVERFLOW;
            }
            currentState = STATE_TEMP;
            break;
        }

        case CMD_START_AUTO: // 0x13
        {
            uint8_t v_period = rx_buf[5];
            uint8_t t_period = rx_buf[6];
            if (v_period < 1 || v_period > 255 || t_period < 1 || t_period > 255) {
                return ERR_PARAM_ERROR;
            }
            volt_period = v_period * PERIOD_UNIT;
            temp_period = t_period * PERIOD_UNIT;
            if (volt_period < 5 || temp_period < 5) {
                buf_overflow_cnt++;
                return ERR_OVERFLOW;
            }
            currentState = STATE_AUTO;
            break;
        }

        default:
            return ERR_INVALID_CMD;
    }

    return err;
}

// ====================== 硬件初始化  ===========================

// 串口+ADC初始化
void Hardware_Init(void) {
    WDTCTL = WDTPW + WDTHOLD;

    // 配置时钟：SMCLK=1MHz（内部校准DCO）
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    // 串口初始化
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;

    UCA0CTL1 |= UCSSEL_2;
    UCA0BR0 = 104;
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS0;
    UCA0CTL1 &= ~UCSWRST;

    IE2 |= UCA0RXIE;
    __bis_SR_register(GIE); // 开启全局中断

    // ADC通道初始化（P1.0/3/4）
    ADC10AE0 |= BIT0 + BIT3 + BIT4;
}

// 毫秒级延时（适配1MHz SMCLK）
void DelayMs(uint16_t ms) {
    for (; ms > 0; ms--) {
        __delay_cycles(1000);
    }
}

// ====================== 中断服务程序（含溢出保护） ======================
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {
    uint8_t rx_byte = UCA0RXBUF;
    // 1. 缓冲区溢出保护（触发EVENT_BUF_OVERFLOW）
    if (rx_len >= RX_BUF_SIZE) {
        rx_len = 0;
        buf_overflow_cnt++; // 标记溢出
        return;
    }

    // 2. 帧头同步：连续收到0xAA→0x55
    if (rx_len == 0) {
        if (rx_byte != FRAME_HEADER1) return;
        rx_buf[rx_len++] = rx_byte;
    } else if (rx_len == 1) {
        if (rx_byte != FRAME_HEADER2) {
            rx_len = 0;
            return;
        }
        rx_buf[rx_len++] = rx_byte;
    } else {
        rx_buf[rx_len++] = rx_byte;
    }
    // 唤醒LPM0
    __bic_SR_register_on_exit(LPM0_bits);
}

// ====================== 主函数  ===========================
int main(void) {
    // 硬件初始化
    Hardware_Init();
    DelayMs(100);
    __bis_SR_register(GIE);

    // 主循环：解析4个核心命令 → 采集 → 发送数据/事件帧
    while (1) {
        // 处理PC命令帧
        if (rx_len >= 7 && rx_buf[rx_len-1] == FRAME_TAIL) {
            ErrorCode err = ParseCmdFrame();
            PackAndSendAckFrame(((CmdFrame *)rx_buf)->seq, err);
            rx_len = 0;
            memset(rx_buf, 0, RX_BUF_SIZE);
        }

        // 按状态采集并发送数据帧
        switch (currentState) {
            case STATE_VOLTAGE:
                PackAndSendVoltFrame();
                DelayMs(volt_period);
                break;

            case STATE_TEMP:
                PackAndSendTempFrame();
                DelayMs(temp_period);
                break;

            case STATE_AUTO:
                PackAndSendDualFrame();
                DelayMs((volt_period + temp_period) / 2);
                break;

            case STATE_IDLE:
            default:
                __bis_SR_register(LPM0_bits); // 空闲时低功耗
                break;
        }
    }
}
