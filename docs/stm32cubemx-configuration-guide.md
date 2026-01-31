# CubeMX Configuration Guide: ADC Board (Piano Velocity)

This guide details the STM32H743ZIT6 configuration for high-speed acquisition on 22 analog channels by leveraging the parallelism of the 3 ADCs with circular DMA.

**Objective**: Simultaneous acquisition at 75-80 kHz on 22 velocity sensors.

---

## 1. Pin Naming (Labels)
**[`Pinout View` Window]**

*Right-click on each cited pin, choose "Enter User Label" and type the exact name.*

### Sensor Inputs (Analog)
- **PB1**: `SEN1` (ADC1_INP5)
- **PB0**: `SEN2` (ADC2_INP9)
- **PC5**: `SEN3` (ADC1_INP8)
- **PC4**: `SEN4` (ADC2_INP4)
- **PA7**: `SEN5` (ADC1_INP7)
- **PA6**: `SEN6` (ADC2_INP3)
- **PA5**: `SEN7` (ADC1_INP19)
- **PA4**: `SEN8` (ADC2_INP18)
- **PA3**: `SEN9` (ADC1_INP15)
- **PA2**: `SEN10` (ADC2_INP14)
- **PA1**: `SEN11` (ADC1_INP17)
- **PA0**: `SEN12` (ADC1_INP16)
- **PF9**: `SEN13` (ADC3_INP2)
- **PF10**: `SEN14` (ADC3_INP6)
- **PC1**: `SEN15` (ADC2_INP11)
- **PC0**: `SEN16` (ADC2_INP10)
- **PF6**: `SEN17` (ADC3_INP8)
- **PF7**: `SEN18` (ADC3_INP3)
- **PF8**: `SEN19` (ADC3_INP7)
- **PF5**: `SEN20` (ADC3_INP4)
- **PF3**: `SEN21` (ADC3_INP5)
- **PF4**: `SEN22` (ADC3_INP9)

### CAN Communication (TJA1042 Module)
- **PA11**: `FDCAN1_RX`
- **PA12**: `FDCAN1_TX`
- **PA9**: `CAN_STB` (GPIO_Output)

### USB (Optional)
- **PB15**: `USB_DP` (USB_OTG_HS_DP)
- **PB14**: `USB_DM` (USB_OTG_HS_DM)
- **PB13**: `USB_VBUS` (USB_OTG_HS_VBUS)

### Console & USART
- **PB6**: `VCP_TX` (USART1_TX)
- **PB7**: `VCP_RX` (USART1_RX)
- **PD5**: `USART2_TX` (USART2_TX)
- **PD6**: `USART2_RX` (USART2_RX)
- **PD8**: `USART3_TX` (USART3_TX)
- **PD9**: `USART3_RX` (USART3_RX)

### System & Debug
- **PA13**: `SWDIO` (Debug)
- **PA14**: `SWCLK` (Debug)
- **PG0**: `TIA_SHDN` (GPIO_Output)

---

## 2. Clock Configuration (Clock Tree)

1. **HSE Input**:
   * **[`System Core` > `RCC`]**
   * Set `High Speed Clock (HSE)` to **Crystal/Ceramic Resonator**.

2. **Clock Tree Tab**:
   * **[`Clock Configuration` tab (at the top)]**
   * **Input frequency (HSE)**: `16` MHz.
   * **System Clock Mux**: PLLCLK
   * **SYSCLK**: Type `440` MHz (or your target frequency) and let CubeMX solve the PLLs.
   * **PLL2P (ADC Source)**: Adjust the multiplier/divider to obtain **80 MHz**.
      * *The ADC will run at 40 MHz after a division by 2 in its internal configuration.*
   * **ADC Clock Mux**: Select `PLL2P` in the bottom-right selector.
   * **FDCAN Clock Mux**: Choose `PLL1Q` to obtain a stable frequency (e.g., 80 MHz).

---

## 3. Debug & SysTick Configuration

**[`Trace and Debug` > `DEBUG`]**

* **Debug**: `Serial Wire`.

*Very important, otherwise you risk losing control over the MCU after the first flash.*

**[`System Core` > `SYS`]**

* **SysTick**: `TIM5`.

---

## 3.1 Timestamp Timer Configuration (TIM2)

Objective: provide a 32-bit monotonic counter for timestamping acquisition frames (reading `TIM2->CNT`), without interrupts and with minimal CPU overhead.

**[`Timers` > `TIM2`]**

In the **Mode** tab:

- **Clock Source**: `Internal Clock`

In the **Parameter Settings** tab:

- **Counter Mode**: `Up`
- **Auto-reload preload**: `Disable`
- **Prescaler (PSC)**: 239 (with a timer clock at 240 MHz, this gives 240 / (239 + 1) = 1 MHz (1 tick = 1 µs)).
- **Counter Period**: `4294967295` (0xFFFFFFFF)

In the **NVIC Settings** tab:

- Do not enable TIM2 interrupts.

Notes:

- TIM2 must remain a "free" timer: it is not used for SysTick (already on TIM5) or PWM generation.
- The counter is modulo 2³², so it "wraps" regularly. This is expected: timestamps will be compared using modulo difference.
- After CubeMX regeneration, the firmware must start TIM2 once at boot (e.g., `HAL_TIM_Base_Start(&htim2)`), without IRQ.

---

## 4. ADC Configuration

### A. General Settings
**[`Analog` > `ADC1`]**

In the **Parameter Settings** tab:

* **Mode**: `Dual Regular Simultaneous mode only`.
* **Clock Prescaler**: `Asynchronous clock divided by 1`
* **Resolution**: `16 bits`.
* **Number of conversions**: `7`
* **Scan Conversion Mode**: `Enabled`. (automatic switching)
* **Continuous Conversion Mode**: `Enabled`.

In the **DMA Settings** tab:

1. Click `Add`, select `ADC1`.
2. **Mode**: `Circular`.
3. **Data Width (Memory)**: `Word` (32-bit).
4. **Data Width (Peripheral)**: `Word` (32-bit).

In the **Parameter Settings** tab:

* **Conversion Data Management Mode**: `DMA Circular mode`
* **End of Conversion Selection**: `End of sequence conversion`.
* **Overrun behavior**: `Overrun data preserved`.

In **ADC_Regular_ConversionMode** / **Ranks**:

- For each **Rank**, choose the associated **Channel**.
- Set **Sampling Time** to `64.5 Cycles` for ALL channels.

| Rank | ADC1 (Master) | Sensor |
|:-----|:--------------|:-------|
| **1** | Channel 5 | **SEN1** |
| **2** | Channel 8 | **SEN3** |
| **3** | Channel 7 | **SEN5** |
| **4** | Channel 19 | **SEN7** |
| **5** | Channel 15 | **SEN9** |
| **6** | Channel 17 | **SEN11** |
| **7** | Channel 16 | **SEN12** |


**[`Analog` > `ADC2`]**

In the **Parameter Settings** tab:

* **Mode**: `Dual Regular Simultaneous mode only`.
* **Clock Prescaler**: `Asynchronous clock divided by 1` 
* **Resolution**: `16 bits`.
* **Number of conversions**: `7`
* **Scan Conversion Mode**: `Enabled`. (automatic switching)
* **Continuous Conversion Mode**: `Enabled`.
* **End of Conversion Selection**: `End of sequence conversion`.
* **Overrun behavior**: `Overrun data preserved`.

In **ADC_Regular_ConversionMode** / **Ranks**:

- For each **Rank**, choose the associated **Channel**.
- Set **Sampling Time** to `64.5 Cycles` for ALL channels.

| Rank | ADC2 (Slave) | Sensor |
|:-----|:-------------|:-------|
| **1** | Channel 9 | **SEN2** |
| **2** | Channel 4 | **SEN4** |
| **3** | Channel 3 | **SEN6** |
| **4** | Channel 18 | **SEN8** |
| **5** | Channel 14 | **SEN10** |
| **6** | Channel 11 | **SEN13** |
| **7** | Channel 10 | **SEN14** |


**[`Analog` > `ADC3`]**

In the **Parameter Settings** tab:

* **Clock Prescaler**: (The parameter does not exist for ADC3, which is why we set a prescaler of 1 for ADC1+2. Everyone will be at the same speed.)
* **Resolution**: `16 bits`.
* **Number of conversions**: `8`
* **Scan Conversion Mode**: `Enabled`.
* **Continuous Conversion Mode**: `Enabled`.

In the **DMA Settings** tab:

1. Click `Add`, select `ADC3`.
2. **Mode**: `Circular`.
3. **Data Width (Memory)**: `Half Word` (16-bit).
4. **Data Width (Peripheral)**: `Half Word` (16-bit).

In the **Parameter Settings** tab:

* **Conversion Data Management Mode**: `DMA Circular mode`
* **End of Conversion Selection**: `End of sequence conversion`.
* **Overrun behavior**: `Overrun data preserved`.

In **ADC_Regular_ConversionMode** / **Ranks**:

- For each **Rank**, choose the associated **Channel**.
- Set **Sampling Time** to `64.5 Cycles` for ALL channels.

| Rank | ADC3 Channel | Sensor |
|:-----|:-------------|:-------|
| **1** | Channel 2 | **SEN13** |
| **2** | Channel 6 | **SEN14** |
| **3** | Channel 8 | **SEN17** |
| **4** | Channel 3 | **SEN18** |
| **5** | Channel 7 | **SEN19** |
| **6** | Channel 4 | **SEN20** |
| **7** | Channel 5 | **SEN21** |
| **8** | Channel 9 | **SEN22** |

---

## 7. Console Configuration (USART1)

**[`Connectivity` > `USART1`]**

1. **Mode**: `Asynchronous`.
2. **Settings**:
   * **Baud Rate**: `115200 Bits/s`.
   * **Word Length**: `8 Bits`.
   * **Parity**: `None`.
   * **Stop Bits**: `1`.
3. **DMA Settings**:
   1. Click `Add` and add **two requests**:
      - `USART1_RX` (Peripheral-to-Memory)
      - `USART1_TX` (Memory-to-Peripheral)
   2. For **USART1_RX**:
      - **Mode**: `Circular`
      - **Data Width (Memory)**: `Byte`
      - **Data Width (Peripheral)**: `Byte`
   3. For **USART1_TX**:
      - **Mode**: `Normal`
      - **Data Width (Memory)**: `Byte`
      - **Data Width (Peripheral)**: `Byte`
4. **NVIC Settings**:
   - Enable `USART1 global interrupt`.
   - Enable interrupts for the **DMA streams** associated with `USART1_RX` and `USART1_TX`.
   - Set the UART/DMA priority to **5** (or lower in urgency) and keep **ADC/FDCAN at a higher priority**.

*USART1 is connected to the ST-Link Virtual COM Port (STDC14) via pins PB6/PB7.*

### USART2 and USART3 (Optional)
**[`Connectivity` > `USART2` / `USART3`]**

These additional serial ports are available on dedicated connectors. Configure them only if your application needs them.

If you use them, repeat the configuration above with the same parameters (115200 bps, 8N1).

*Verify in Pinout View that the pins match the labels created in step 1.*

---

## 8. USB Configuration (Optional)

**[`Connectivity` > `USB_OTG_HS`]**

1. **External PHY**: `Disable`.
2. **Internal FS PHY**: `Device_Only`.
3. **Speed**: `Full Speed 12MBit/s`.

*Note: This board uses the OTG_HS peripheral with the internal Full-Speed PHY. USB configuration is optional depending on the project's needs.*

---

## 9. CAN Bus Configuration (FDCAN1)

**[`Connectivity` > `FDCAN1`]**

1. **Mode**: `Normal`.
2. **Frame Format**: `Classic mode` (to match other boards on the network).
3. **Timings (for 500 kbit/s with FDCAN clock at 80MHz)**:
   * **Nominal Prescaler**: `10`.
   * **Nominal Time Seg1**: `12`.
   * **Nominal Time Seg2**: `3`.
   * **Nominal Sync Jump Width**: `1`.

4. **Buffers** (temporary settings, adjust as needed):
   * **Tx Buffers Nbr**: 0
   * **Tx Fifo Queue Elmts Nbr**: 32
   * **Std Filters Nbr**: 1
   * **Rx Fifo0 Elmts Nbr**: 32

5. **NVIC Settings**: Enable `FDCAN1 interrupt 0` and `FDCAN1 interrupt 1`.

*Note: These timing values depend on the frequency set for the FDCAN clock in the Clock Configuration tab.*

---

## 10. GPIO Initialization & Default State

**[`System Core` > `GPIO`]**

1. **TIA_SHDN (PG0)**:
   * **GPIO Output Level**: `Low`.
   * *The transimpedance amplifiers will be off at startup (Standby mode).*

2. **CAN_STB (PA9)**:
   * **GPIO Output Level**: `Low`.
   * *The CAN transceiver will be in Normal mode (active) at startup.*

---

## 11. Enable FreeRTOS

**[`Middlewares and Software Packs` > `FREERTOS`]**

1. **Interface**: `CMSIS_V2`.
2. **Tasks and Queues**: Increase the stack size of `defaultTask` to at least **1024 words**.
   * *Why? Peripheral initialization takes place in this task and consumes a lot of stack. A value that is too low causes an immediate HardFault at startup.*

---

## 12. Project Generation (Project Manager)

**[`Project Manager` tab]**

1. **Project Name**: `H743_ADC_Board`
2. **Toolchain / IDE**: `CMake`.
3. **Code Generator**:
   * **Library Files**: `Copy only the necessary library files`.
   * **File Management**: `Generate peripheral initialization as a pair of '.c/.h' files per peripheral`.
   * **User Code**: `Keep User Code when re-generating`

4. Click `GENERATE CODE`.
