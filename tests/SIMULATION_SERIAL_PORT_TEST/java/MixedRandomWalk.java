package com.example.walk;

import com.fazecast.jSerialComm.SerialPort;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

public class MixedRandomWalk {
    private static final int BASE_VOLTAGE = 2500;
    private static final int DRIFT_STEP = 4;
    private static final int SMALL_JITTER = 10;
    private static final int LARGE_JUMP = 200;
    private static final double JUMP_PROB = 0.1;

    // 温度范围：-70°C ~ +70°C (int16_t, 单位 0.01°C)
    private static final int TEMP_MIN = -7000;
    private static final int TEMP_MAX = 7000;
    private static int temperature = 2000;  // 初始 20°C

    private static final Random random = new Random();

    private static int voltage = BASE_VOLTAGE;
    private static int driftDirection = 1;
    private static byte dataSeq = 0;  // 数据帧序号

    // 工作模式
    private enum WorkMode {
        IDLE, VOLTAGE, TEMPERATURE, DUAL
    }

    private static AtomicReference<WorkMode> currentMode = new AtomicReference<>(WorkMode.IDLE);
    private static AtomicInteger sendPeriodMs = new AtomicInteger(50);
    private static volatile boolean running = true;

    public static void main(String[] args) throws Exception {
        String portName = "COM3";
        SerialPort port = SerialPort.getCommPort(portName);
        port.setComPortParameters(115200, 8, SerialPort.ONE_STOP_BIT, SerialPort.NO_PARITY);
        port.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 100, 0);

        if (!port.openPort()) {
            System.out.println("无法打开串口：" + portName);
            return;
        }
        System.out.println("已打开串口：" + portName + " @ 115200");

        OutputStream out = port.getOutputStream();
        InputStream in = port.getInputStream();

        // 启动接收线程
        Thread recvThread = new Thread(() -> receiveLoop(in, out));
        recvThread.setDaemon(true);
        recvThread.start();

        // 主发送线程
        sendLoop(out);

        port.closePort();
    }

    private static void sendLoop(OutputStream out) throws Exception {
        while (running) {
            WorkMode mode = currentMode.get();
            int period = sendPeriodMs.get();

            switch (mode) {
                case VOLTAGE:
                    updateVoltage();
                    byte[] vFrame = buildDataFrame((byte) 0x01, voltage, 0);
                    out.write(vFrame);
                    out.flush();
                    break;

                case TEMPERATURE:
                    updateTemperature();
                    byte[] tFrame = buildDataFrame((byte) 0x02, 0, temperature);
                    out.write(tFrame);
                    out.flush();
                    break;

                case DUAL:
                    updateVoltage();
                    updateTemperature();
                    byte[] dFrame = buildDualFrame();
                    out.write(dFrame);
                    out.flush();
                    break;

                case IDLE:
                default:
                    Thread.sleep(50);
                    continue;
            }

            Thread.sleep(period);
        }
    }

    private static void receiveLoop(InputStream in, OutputStream out) {
        byte[] buffer = new byte[256];
        int pos = 0;

        try {
            while (running) {
                int available = in.available();
                if (available > 0) {
                    int read = in.read(buffer, pos, Math.min(available, buffer.length - pos));
                    if (read > 0) {
                        System.out.print("🔵 接收到原始数据: ");
                        printHex(buffer, pos + read);
                        pos += read;
                        pos = processBuffer(buffer, pos, out);
                    }
                } else {
                    Thread.sleep(10);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static int processBuffer(byte[] buffer, int length, OutputStream out) {
        int i = 0;
        while (i < length) {
            // 查找帧头 AA 55
            if (i + 1 < length && (buffer[i] & 0xFF) == 0xAA && (buffer[i + 1] & 0xFF) == 0x55) {
                if (i + 2 < length) {
                    int frameLen = buffer[i + 2] & 0xFF;

                    if (i + frameLen <= length) {
                        byte[] frame = new byte[frameLen];
                        System.arraycopy(buffer, i, frame, 0, frameLen);
                        System.out.print("🔍 提取完整帧: ");
                        printHex(frame, frameLen);
                        handleFrame(frame, out);
                        i += frameLen;
                        continue;
                    } else {
                        // 帧不完整，保留到下次
                        System.out.printf("⏳ 帧不完整: 需要%d字节, 当前只有%d字节\n", frameLen, length - i);
                        break;
                    }
                } else {
                    break;
                }
            }
            i++;
        }

        // 移动未处理数据到缓冲区开头
        if (i < length) {
            System.arraycopy(buffer, i, buffer, 0, length - i);
            return length - i;
        }
        return 0;
    }

    private static void handleFrame(byte[] frame, OutputStream out) {
        if (frame.length < 7) return;

        int len = frame[2] & 0xFF;
        int type = frame[3] & 0xFF;
        byte seq = frame[4];

        // 校验帧长度
        if (frame.length != len) {
            System.out.printf("❌ 帧长度不匹配: 期望=%d, 实际=%d | ", len, frame.length);
            printHex(frame, frame.length);
            return;
        }

        // 校验 CRC (从byte[2]到byte[len-2])
        byte receivedCrc = frame[len - 2];
        byte calculatedCrc = calcCRC8(frame, 2, len - 2);

        if (receivedCrc != calculatedCrc) {
//            System.out.printf("❌ CRC校验失败: 收到=%02X, 计算=%02X | ",
//                    receivedCrc & 0xFF, calculatedCrc & 0xFF);
//            printHex(frame, frame.length);
            //return;
        }

        try {
            switch (type) {
                case 0x11: // 启动电压模拟
                    if (len == 8) {
                        int period = (frame[5] & 0xFF) * 5;
                        currentMode.set(WorkMode.VOLTAGE);
                        sendPeriodMs.set(period);
                        System.out.printf("📊 启动电压模拟 | 周期=%dms | ", period);
                        printHex(frame, frame.length);
                        sendAck(out, seq);
                    }
                    break;

                case 0x12: // 启动温度模拟
                    if (len == 8) {
                        int period = (frame[5] & 0xFF) * 5;
                        currentMode.set(WorkMode.TEMPERATURE);
                        sendPeriodMs.set(period);
                        System.out.printf("🌡️  启动温度模拟 | 周期=%dms | ", period);
                        printHex(frame, frame.length);
                        sendAck(out, seq);
                    }
                    break;

                case 0x13: // 启动双通道模拟
                    if (len == 9) {
                        int vPeriod = (frame[5] & 0xFF) * 5;
                        int tPeriod = (frame[6] & 0xFF) * 5;
                        currentMode.set(WorkMode.DUAL);
                        sendPeriodMs.set(vPeriod); // 使用V_PERIOD
                        System.out.printf("📊🌡️  启动双通道模拟 | V周期=%dms, T周期=%dms | ",
                                vPeriod, tPeriod);
                        printHex(frame, frame.length);
                        sendAck(out, seq);
                    }
                    break;

                case 0x10: // 停止模拟
                    if (len == 7) {
                        WorkMode prev = currentMode.getAndSet(WorkMode.IDLE);
                        System.out.printf("⏹️  停止模拟 (之前=%s) | ", prev);
                        printHex(frame, frame.length);
                        sendAck(out, seq);
                    }
                    break;

                default:
                    System.out.printf("❓ 未知命令类型 0x%02X | ", type);
                    printHex(frame, frame.length);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static void sendAck(OutputStream out, byte seq) throws Exception {
        byte[] ack = new byte[9];
        ack[0] = (byte) 0xAA;
        ack[1] = (byte) 0x55;
        ack[2] = 0x08;
        ack[3] = 0x0F;
        ack[4] = seq;
        ack[5] = 0x00;
        ack[6] = calcCRC8(ack, 2, 6);
        ack[7] = 0x0D;

        out.write(ack);
        out.flush();

        System.out.print("✅ 应答: ");
        printHex(ack, 8);
    }

    private static void updateVoltage() {
        voltage += driftDirection * DRIFT_STEP
                + random.nextInt(SMALL_JITTER * 2 + 1) - SMALL_JITTER;

        if (voltage > 3300 || voltage < 1700)
            driftDirection *= -1;

        if (random.nextDouble() < JUMP_PROB) {
            voltage += (random.nextBoolean() ? 1 : -1) * LARGE_JUMP;
        }

        voltage = Math.max(0, Math.min(voltage, 4095));
    }

    private static void updateTemperature() {
        temperature += random.nextInt(800) - 400;  // 小幅抖动

        if (random.nextDouble() < 0.05) {
            temperature += (random.nextBoolean() ? 1000 : -1000); // 偶发跳变 ±10°C
        }

        temperature = Math.max(TEMP_MIN, Math.min(TEMP_MAX, temperature));
    }

    private static byte[] buildDataFrame(byte type, int voltage, int temperature) {
        byte[] frame = new byte[9];
        frame[0] = (byte) 0xAA;
        frame[1] = (byte) 0x55;
        frame[2] = 0x09;  // 帧总长度
        frame[3] = type;
        frame[4] = dataSeq++;

        int value = (type == 0x01) ? voltage : temperature;
        frame[5] = (byte) (value & 0xFF);        // Low byte
        frame[6] = (byte) ((value >> 8) & 0xFF); // High byte

        frame[7] = calcCRC8(frame, 2, 7);
        frame[8] = 0x0D;

        return frame;
    }

    private static byte[] buildDualFrame() {
        byte[] frame = new byte[11];
        frame[0] = (byte) 0xAA;
        frame[1] = (byte) 0x55;
        frame[2] = 0x0B;  // 帧总长度
        frame[3] = 0x03;
        frame[4] = dataSeq++;

        // 电压 uint16_t LE
        frame[5] = (byte) (voltage & 0xFF);
        frame[6] = (byte) ((voltage >> 8) & 0xFF);

        // 温度 int16_t LE
        frame[7] = (byte) (temperature & 0xFF);
        frame[8] = (byte) ((temperature >> 8) & 0xFF);

        frame[9] = calcCRC8(frame, 2, 9);
        frame[10] = 0x0D;

        return frame;
    }

    private static byte calcCRC8(byte[] data, int start, int end) {
        int crc = 0x00;
        for (int i = start; i < end; i++) {
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
        System.out.println(sb.toString().trim());
    }
}