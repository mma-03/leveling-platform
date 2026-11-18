# Self-Leveling Platform (STM32)
A servo-driven platform that keeps itself level. An IMU measures how far the base is tilted, and a servo counter-rotates the platform to cancel it, 100 times a second. Written in bare-metal C on an STM32F401 (ARM Cortex-M4), with the IMU driver written from the MPU-6500 datasheet and the microcontroller's peripherals accessed through ST's HAL.

## Hardware
| **Part** | **Role** |
|---|---|
| NUCLEO-F401RE (STM32F401RE, Cortex-M4 @ 84 MHz) | Microcontroller |
| GY-521 module (MPU-6500 IMU) | Accelerometer (read over I2C) |
| MG90S servo | Rotates the platform |
| 5V, 3A adapter + 1000 µF capacitor | Separate servo supply, buffered against current spikes |

### Pin Map
| **Pin** | **Function** | **Connected to** |
|---|---|---|
| PA0 | TIM2_CH1 (PWM)| Servo signal |
| PB8 | I2C1_SCL | IMU SCL |
| PB9 | I2C1_SDA | IMU SDA |
| PC13 | EXTI (falling edge) | User calibration button |
| PA2/PA3 | USART2 | ST-LINK virtual COM port (115200 8N1) |

## How it works
Every 10 ms, driven by a hardware timer interrupt:
1. **Read** all three accelerometer axes in a single 6-byte I2C exchange.
2. **Compute the tilt** from how gravity is split between two axes: `atan2f(y, z)`, converted to degrees.
3. **Apply calibration:** subtract the zero offset recorded on the last button press.
4. **Filter** the angle with a low-pass filter (α = 0.2) to suppress sensor noise.
5. **Command the servo** to the opposite angle through a 50 Hz PWM signal with 1 µs resolution.

## Code Structure
Each piece of hardware has its own module, built on top of ST's HAL, with a header describing what it does and a .c file hiding how.

| **Module** | **Responsibility** |
|---|---|
| imu | MPU-6500 driver: identity check, configuration, 6-byte reads, tilt angle, calibration, filtering |
| servo | Angle-to-pulse conversion, safety clamping (600-2400 µs), center trim |
| tick | 10 ms timer tick (TIM3 interrupt) for a fixed-rate control loop |
| button | Interrupt-driven button events with interrupt-safe flag handling |
| serial | printf-style logging over UART |

## Key design decisions
- **Fixed-rate loop:** a timer interrupt sets the loop period, so timing stays constant as the code changes. Estimated budget: ~3 ms of work per 10 ms period.
- **Calibration by averaging:** pressing the button averages 100 raw readings (~1 s), reducing noise in the zero point by about 10×. Calibration always averages the raw angle, so recalibrating never compounds an old offset.
- **Filter tuning:** α = 0.2, chosen from calculated noise/lag estimates (predicted ~2/3 noise reduction and ~50 ms of lag). Measured with the board still: the spread of readings dropped from ~0.72° to ~0.37°, and the servo command stopped jumping between positions.
- **Mounting direction as a constant:** the servo's mounting reverses its direction relative to the IMU. This lives in one named constant rather than being hidden in the control logic.
- **Interrupt safety:** flags shared with interrupts are volatile, and read-and-clear happens inside a critical section to avoid a lost-event race condition.

## Verification
- IMU verified on hardware: the tilt angle tracked the board in both directions, and the measured gravity vector totalled ~1.0 g (Y = 0.42 g, Z = 0.91 g at a 24.7° tilt).
- Servo endpoints and center measured on hardware. 1 µs pulse resolution = ~0.09°.
- Calibration centers the reading on ~0.0°, and repeated calibration gives the same result.
- Low-pass filter tested with the board still, filter off vs on: the spread was significantly reduced (~0.72° to ~0.37°).
- Full test results for each stage are in the design notes.

## Design notes
Detailed design decisions, datasheet analysis, and test results for each module are in `docs/`, including the integration notes.

## Building and Flashing
Built with CMake and Ninja using the STM32CubeIDE extension for VS Code (STM32CubeMX for peripheral configuration). Build with the extension's build command, and flash and debug through the ST-LINK GDB server.

## Roadmap
- **OLED display** (SSD1306, sharing the I2C bus with the IMU): show the live tilt angle. Wired; driver in progress.
- **Gyroscope fusion** (complementary filter): keep the angle accurate during fast movement, when acceleration other than gravity distorts the accelerometer reading.
- **FreeRTOS port:** split sensing, control, and display into tasks.
- **DMA** for I2C transfers, and a second servo axis.
