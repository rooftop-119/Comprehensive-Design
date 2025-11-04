package com.example.walk;

import com.fazecast.jSerialComm.SerialPort;

import java.util.Random;

public class OriginalWalk {
    public static void walk() throws InterruptedException {

        SerialPort port = SerialPort.getCommPort("COM3");
        port.setBaudRate(115200);
        port.openPort();

        System.out.println("使用串口: " + port.getSystemPortName());

        byte seq = 0;
        Random rand = new Random();

        while (true) {
            // 构建数据帧
            byte[] frame = new byte[9];
            frame[0] = (byte) 0xAA;
            frame[1] = (byte) 0x55;
            frame[2] = 0x01;        // 类型
            frame[3] = seq;         // 序号
            int data = rand.nextInt(65536);  // 随机 0~65535
            frame[4] = (byte) (data & 0xFF);       // 低字节
            frame[5] = (byte) ((data >> 8) & 0xFF);// 高字节
            frame[6] = 0x00;        // 状态码
            frame[7] = calcCRC8(frame, 2, 5); // CRC8，计算类型到状态码
            frame[8] = 0x0D;        // 尾

            // 发送
            port.writeBytes(frame, frame.length);

            System.out.printf("发送: seq=%d, data=%d\n", seq & 0xFF, data);

            seq++; // 序号自增
            Thread.sleep(500); // 每500ms发送一帧，可根据需求调整
        }
    }

    // 简单 CRC8 计算（多项式 0x07）
    private static byte calcCRC8(byte[] data, int offset, int length) {
        byte crc = 0x00;
        for (int i = offset; i <= offset + length; i++) {
            crc ^= data[i];
            for (int j = 0; j < 8; j++) {
                if ((crc & 0x80) != 0) {
                    crc = (byte) ((crc << 1) ^ 0x07);
                } else {
                    crc <<= 1;
                }
            }
        }
        return crc;
    }
}