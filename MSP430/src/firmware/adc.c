#include <msp430.h>
#include <stdint.h>
#include <stdlib.h>

#define NUM_SAMPLES 200 
#define LED_PIN BIT0

// 采样数组
uint16_t ad[NUM_SAMPLES]; 

// 滑动窗口积分和
uint32_t sum_60 = 0; // 修改为 uint32_t 防止 64 个点累加溢出 (1023*64 = 65472，接近 uint16 极限)

// 触发标志位
volatile uint8_t capture_true = 0; 
volatile uint8_t wait_for_r_flag = 1;

// UART 通信函数
void UART_Init(void) {
    P1SEL |= BIT1 + BIT2; 
    P1SEL2 |= BIT1 + BIT2;
    UCA0CTL1 |= UCSSEL_2; 
    UCA0BR0 = 104; // 9600 bps @ 1MHz SMCLK
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS0; 
    UCA0CTL1 &= ~UCSWRST; 
    IE2 |= UCA0RXIE; 
}

void UART_SendChar(char c) {
    while (!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = c; 
}

void UART_SendString(char *str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

// 高性能数字转字符串函数
void UART_SendU16(uint16_t val) {
    //  buf[5] -> buf[6]，防止 5 位数 + 结束符溢出
    char buf[6]; 
    int i = 5;
    
    if (val == 0) {
        UART_SendChar('0');
        return;
    }
    
    buf[5] = '\0';
    while (val > 0 && i > 0) {
        i--;
        buf[i] = (val % 10) + '0';
        val /= 10;
    }
    UART_SendString(&buf[i]);
}

void UART_SendADC_Data(void) {
    int i;
    UART_SendString("ADC Data=:\r\n");
    for (i = 0; i < NUM_SAMPLES; i++) {
        UART_SendU16(ad[i]);
        if (i < NUM_SAMPLES - 1) {
            UART_SendString(", ");
        }
        if ((i + 1) % 8 == 0) {
            UART_SendString("\r\n");
        }
        // 适当减小延迟以加快刷新，原 5000 可能太慢
        __delay_cycles(2000); 
    }
    UART_SendString("End!\r\n"); 
}

// ================= 核心 ADC 采集循环函数 =================
void adc_sampling_process(){
    uint16_t adtemp; 
    int16_t j;
    int16_t i; 
    
    //  连续刷新必须重置这些状态变量
    capture_true = 0; 
    sum_60 = 0; 

    ADC10CTL0 &= ~ADC10IE; 
    
    for (i = 0; i < NUM_SAMPLES; i++)
    {
        ADC10CTL0 |= ENC + ADC10SC;
        while (ADC10CTL0 & ADC10SC); // 等待转换完成
        adtemp = ADC10MEM;

        if (capture_true)
        {
            // 触发后，直接记录数据
            ad[i] = adtemp;
        }
        else
        { 
            // 触发前：填充缓冲区并计算滑动平均
            if (i <= 63)
            { 
                ad[i] = adtemp;
                sum_60 += adtemp; 
            }
            
            if (i > 63)
            {
                // 触发条件判断：波形突变检测
                // abs((平均值) - 当前值) > 50 且 当前值 > 0xda
                if ((abs((sum_60 >> 6) - adtemp) > 50) && (adtemp > 0xda))
                { 
                    ad[i] = adtemp;
                    capture_true = 1; // 标记已触发
                }
                else 
                {
                    // 未触发：滑动窗口左移
                    // 注意：这里效率较低，如果是高频采样建议优化算法
                    sum_60 -= ad[0];
                    for (j = 0; j < 63; j++) 
                        ad[j] = ad[j + 1];
                    ad[63] = adtemp; 
                    sum_60 += adtemp;
                    i--; // i 不自增，保持窗口监测状态
                }
            }
        }
    }
}

// 中断服务程序
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void) {
    __no_operation();
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {
    char rxData = UCA0RXBUF;
    // 回显
    if (rxData != '\r' && rxData != '\n') {
        UART_SendChar(rxData);
    }
    // 收到 'r' 启动
    if (rxData == 'r') {
        wait_for_r_flag = 0;
        __bic_SR_register_on_exit(LPM3_bits); 
    }
}

//  主函数
int main(void){
    WDTCTL = WDTPW | WDTHOLD;
    
    // 时钟初始化 1MHz
    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    
    // GPIO 初始化
    P1DIR = 0xFF; 
    P1OUT = 0;
    P1DIR &= ~(BIT7 + BIT1); // P1.7 (A7) 和 P1.1 (RXD) 输入
    P1DIR |= LED_PIN; 
    P1OUT |= LED_PIN; 
    
    UART_Init();
    
    // ADC 初始化
    ADC10CTL0 = SREF_0 + ADC10SHT_2 + ADC10ON;
    ADC10CTL1 = INCH_7 + SHS_0 + ADC10DIV_0 + CONSEQ_0; 
    ADC10AE0 |= BIT7; 
    
    __enable_interrupt(); 
    UART_SendString("Starting ADC\r\n"); 
    UART_SendString("Pls Re<r>...C\r\n");
    __delay_cycles(1000000);

    // 等待第一次 'r' 键按下
    while(wait_for_r_flag) {
        _bis_SR_register(LPM3_bits + GIE);
    }
    
    P1OUT &= ~LED_PIN; 
    
    //  无限循环，实现连续刷新
    while(1)
    {
        // 1. 采集 (内部已包含等待触发逻辑)
        adc_sampling_process();
        
        // 2. 发送数据
        UART_SendADC_Data();
        
        // 3. 两次采集间的间隔，防止串口助手卡死
        // 约 50ms 延时
        __delay_cycles(50000); 
    }
}