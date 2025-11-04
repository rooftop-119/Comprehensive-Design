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
    private static final int FRAME_INTERVAL_MS = 100; // 发送间隔,100

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

            byte[] frame = buildFrame((byte) seq++, voltage);
            out.write(frame);
            out.flush();

            System.out.printf("发送帧 序号=%3d 电压=%4dmV\n", frame[3] & 0xFF, voltage);
            Thread.sleep(FRAME_INTERVAL_MS);
        }
    }

    private static byte[] buildFrame(byte seq, int voltage) {
        byte[] frame = new byte[9];
        frame[0] = (byte) 0xAA;
        frame[1] = (byte) 0x55;
        frame[2] = 0x01; // 类型
        frame[3] = seq;
        frame[4] = (byte) (voltage & 0xFF);
        frame[5] = (byte) ((voltage >> 8) & 0xFF);
        frame[6] = 0x00; // 状态码
        frame[7] = calcCRC8(frame, 7); // CRC8
        frame[8] = 0x0D; // 尾
        return frame;
    }

    private static byte calcCRC8(byte[] data, int len) {
        byte crc = 0x00;
        for (int i = 0; i < len; i++) {
            crc ^= data[i];
            for (int j = 0; j < 8; j++) {
                if ((crc & 0x80) != 0)
                    crc = (byte) ((crc << 1) ^ 0x07);
                else
                    crc <<= 1;
            }
        }
        return crc;
    }
}
