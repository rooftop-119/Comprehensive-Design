package com.example.walk;

import com.fazecast.jSerialComm.SerialPort;
import java.io.OutputStream;
import java.util.Random;

public class MixedRandomWalk {
    private static final int BASE_VOLTAGE = 2500; // 初始电压 (mV),2500
    private static final int DRIFT_STEP = 4;      // 每次慢漂步长,2
    private static final int SMALL_JITTER = 10;    // 微扰动范围,5
    private static final int LARGE_JUMP = 200;    // 偶发大波动,100
    private static final double JUMP_PROB = 0.1; // 大跳变概率,0.05

    // 温度范围：6~60（最终转 uint16_t）
    private static final int TEMP_MIN = 6000;
    private static final int TEMP_MAX = 60000;
    private static int temperature = 20000;     // 初始温度

    private static final int FRAME_INTERVAL_MS = 50; // 发送间隔,100
    private static final Random random = new Random();

    private static int seq = 0;
    private static int voltage = BASE_VOLTAGE;
    private static int driftDirection = 1;

    public static void walk() throws Exception{
        String portName = "COM3"; // 修改为你的 COM 号
        SerialPort port = SerialPort.getCommPort(portName);
        port.setComPortParameters(115200, 8, SerialPort.ONE_STOP_BIT, SerialPort.NO_PARITY);
        port.setComPortTimeouts(SerialPort.TIMEOUT_NONBLOCKING, 0, 0);

        if (!port.openPort()) {
            System.out.println("无法打开串口：" + portName);
            return;
        }
        System.out.println("已打开串口：" + portName);

        OutputStream out = port.getOutputStream();

        while (true) {
            // —— 模拟电压变化 ——
            // 轻微随机游走 + 漂移
            voltage += driftDirection * DRIFT_STEP + random.nextInt(SMALL_JITTER * 2 + 1) - SMALL_JITTER;
            // 控制上下限
            if (voltage > 3300 || voltage < 1700) driftDirection *= -1;

            // 偶尔大跳变
            if (random.nextDouble() < JUMP_PROB) {
                voltage += (random.nextBoolean() ? 1 : -1) * LARGE_JUMP;
            }

            // 限制范围
            voltage = Math.max(0, Math.min(voltage, 4095));

            byte[] frame = buildFrame((byte) seq++,(byte) 0x02, voltage);
            out.write(frame);
            out.flush();

            System.out.printf("发送帧 序号=%3d 电压=%4dmV\n", frame[3] & 0xFF, voltage);
            printHex(frame,9);
            Thread.sleep(FRAME_INTERVAL_MS);
        }
    }

    public static void sendMixedFrames() throws Exception {

        String portName = "COM3"; // 修改为实际的 COM
        SerialPort port = SerialPort.getCommPort(portName);
        port.setComPortParameters(115200, 8, SerialPort.ONE_STOP_BIT, SerialPort.NO_PARITY);
        port.setComPortTimeouts(SerialPort.TIMEOUT_NONBLOCKING, 0, 0);

        if (!port.openPort()) {
            System.out.println("无法打开串口：" + portName);
            return;
        }
        System.out.println("已打开串口：" + portName);

        OutputStream out = port.getOutputStream();

        while (true) {

            // --------------------------------------------------------
            // 1. 生成电压数据
            // --------------------------------------------------------
            voltage += driftDirection * DRIFT_STEP
                    + random.nextInt(SMALL_JITTER * 2 + 1) - SMALL_JITTER;

            if (voltage > 3300 || voltage < 1700)
                driftDirection *= -1;

            if (random.nextDouble() < JUMP_PROB) {
                voltage += (random.nextBoolean() ? 1 : -1) * LARGE_JUMP;
            }

            voltage = Math.max(0, Math.min(voltage, 4095));

            // --------------------------------------------------------
            // 2. 生成温度数据（uint16_t）
            // --------------------------------------------------------
            temperature += random.nextInt(8000) - 4000;  // 小幅抖动

            if (random.nextDouble() < 0.05) {
                temperature += (random.nextBoolean() ? 10000 : -10000); // 偶发跳变（±0.1°C）
            }

            temperature = Math.max(TEMP_MIN, Math.min(TEMP_MAX, temperature));

            int tempU16 = temperature & 0xFFFF;

            // --------------------------------------------------------
            // 3. 构造两帧：电压帧 + 温度帧
            // --------------------------------------------------------
            byte[] voltFrame = buildFrame((byte) seq++, (byte) 0x01, voltage);
            byte[] tempFrame = buildFrame((byte) seq++, (byte) 0x02, tempU16);

            // --------------------------------------------------------
            // 4. 一次性发送
            // --------------------------------------------------------
            out.write(voltFrame);
            out.write(tempFrame);
            out.flush();

            // --------------------------------------------------------
            // 5. 打印日志
            // --------------------------------------------------------
            System.out.printf("电压帧: Seq=%3d  %4dmV\n", voltFrame[3] & 0xFF, voltage);
            printHex(voltFrame, voltFrame.length);

            System.out.printf("温度帧: Seq=%3d  %4d (x0.01 °C)\n", tempFrame[3] & 0xFF, tempU16);
            printHex(tempFrame, tempFrame.length);

            Thread.sleep(FRAME_INTERVAL_MS);
        }
    }


    private static byte[] buildFrame(byte seq, byte type, int value) {
        byte[] frame = new byte[9];
        frame[0] = (byte) 0xAA;
        frame[1] = (byte) 0x55;
        frame[2] = type;
        frame[3] = seq;
        frame[4] = (byte) (value & 0xFF);
        frame[5] = (byte) ((value >> 8) & 0xFF);
        frame[6] = 0x00; // 状态
        frame[7] = calcCRC8(frame, 7);
        frame[8] = 0x0D;
        return frame;
    }

    private static byte calcCRC8(byte[] data, int len) {
        int crc = 0x00;

        for (int i = 2; i < len; i++) {
            crc ^= (data[i] & 0xFF);
            for (int j = 0; j < 8; j++) {
                if ((crc & 0x80) != 0)
                    crc = ((crc << 1) ^ 0x07) & 0xFF;
                else
                    crc = (crc << 1) & 0xFF;
            }
        }
        return (byte) crc;
    }


    private static void printHex(byte[] data, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            sb.append(String.format("%02X ", data[i] & 0xFF));
        }
        System.out.println(sb.toString());
    }
}
