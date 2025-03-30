#ifndef __OLED_PANEL_CMDS_H__
#define __OLED_PANEL_CMDS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define OLED_CMD_NOP            -1   // Don't Send Command

#define OLED_CMD_SWRST          0xA5 // Software Reset (the frame buffer is not cleared)

#define OLED_CMD_STANDBY        0x49 // Enter Standby Mode (RST# will be not able to reset the chip)
#define OLED_CMD_WAKEUP         0xAA // Exit Standby Mode

#define OLED_CMD_INT_EN         0x43 // Enable INT (frame synchronized signal)
#define OLED_CMD_INT_DIS        0x42 // Disable INT

#define OLED_CMD_CLK_DIV_1      0x44 // T_PWMCLK = 1 * T_OSC
#define OLED_CMD_CLK_DIV_2      0x45 // T_PWMCLK = 2 * T_OSC
#define OLED_CMD_CLK_DIV_4      0x46 // T_PWMCLK = 4 * T_OSC
#define OLED_CMD_CLK_DIV_8      0x47 // T_PWMCLK = 8 * T_OSC

/**
 * OSC Trim
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_OSC_TRIM
 * - 2nd Byte: b00000[TRIM:4] (OSC Trim)
 * 
 */
#define OLED_CMD_OSC_TRIM       0x4C // Set OSC Frequency (then send 0x0[0-F], the higher the number, the higher the frequency)
#define OLED_SET_OSC_TRIM(TRIM) ((TRIM) & 0x0F)

#define OLED_CMD_COLOR_RGB666   0x30 // Set Color Mode to RGB666
#define OLED_CMD_COLOR_RGB565   0x31 // Set Color Mode to RGB565
#define OLED_CMD_COLOR_MONO_64  0x32 // Set Color Mode to Mono, 64 Gray
#define OLED_CMD_COLOR_MONO_2   0x33 // Set Color Mode to Mono, 2 Gray

#define OLED_CMD_GAM_GREEN      0x28 // Set Green Gamma Curve (followed by different values for each level)
#define OLED_CMD_GAM_BLUE       0x29 // Set Blue Gamma Curve (followed by different values for each level)
#define OLED_CMD_GAM_RED        0x2A // Set Red Gamma Curve (followed by different values for each level, in mono mode, only R curve is used)
#define OLED_CMD_GAM_UPDATE     0x2B // Update Gamma Table

/**
 * Row & Column Address and Row Length
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_ROWADDR
 * - 2nd Byte: b0[data:7]
 * 
 */
#define OLED_CMD_ROWADDR        0x80 // Set Row Address (followed by row address, b0[6:0])
#define OLED_SET_ROWADDR(RA)    ((RA) & 0x7F)
#define OLED_CMD_COLADDR        0x81 // Set Column Address (followed by column address, b0[6:0])
#define OLED_SET_COLADDR(CA)    ((CA) & 0x7F)
#define OLED_CMD_ROWLEN         0x82 // Set Row Length (followed by row length, b0[6:0])
#define OLED_SET_ROWLEN(RL)     ((RL) & 0x7F)

/**
 * Data Read/Write
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_RAMWR/RD
 * - 2nd Byte(Output For RD): Data
 * - 3rd Byte(Output For RD): Data
 * - nth Byte(Output For RD): Data
 * 
 * RGB666:
 * - 2nd Byte: R b00[5:0]
 * - 3rd Byte: G b00[5:0]
 * - 4th Byte: B b00[5:0]
 * RGB565: 
 * - 2nd Byte: R b[4:0] G b[5:3]
 * - 3rd Byte: G b[2:0] B b[4:0]
 * Mono64:
 * - 2nd Byte: b00[5:0]
 * Mono2:
 * - 2nd Byte: b[P7, P6, P5, P4, P3, P2, P1, P0]
 * 
 */
#define OLED_CMD_RAMWR          0x83 // Write Data to Frame Buffer
#define OLED_CMD_RAMRD          0x84 // Read Data from Frame Buffer

#define OLED_CMD_DISP_T1_OFF    0x58 // Display Type 1 OFF (all seg low, all com high)
#define OLED_CMD_DISP_T2_OFF    0x59 // Display Type 2 OFF (all seg low, all com scan normally)
#define OLED_CMD_DISP_ALL_ON    0x5A // Display All ON (all seg high, all com scan normally                                                                   )
#define OLED_CMD_DISP_NORMAL    0x5B // Display Normal (chip scan normally)

/**
 * Screen Mirror
 * 
 * Byte Information:
 * - 1st Byte: b011010[H:1, V:1]
 * 
 */
#define OLED_CMD_MIRROR(H, V)   (0x68 | (!!(H) << 1) | (!!(V)))

#define OLED_CMD_GPH_ACC_EN     0x71 // Enable Graphic Acceleration
#define OLED_CMD_GPH_ACC_DIS    0x70 // Disable Graphic Acceleration

/**
 * Graphic Acceleration Mode
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_GPH_ACC
 * - 2nd Byte: H b000[GH:1, GV:1, GA:3]
 * 
 * GH = 1: Left -> Right
 * GH = 0: Right -> Left
 * 
 * GV = 1: Top -> Bottom
 * GV = 0: Bottom -> Top
 * 
 * GA = b000: Horizontal Scroll
 * GA = b001: Vertical Scroll
 * GA = b010: Horizontal & Vertical Scroll
 * GA = b011: Fade Out
 * GA = b100: Blinking
 * GA = b101: Zoom In
 * GA = Other: Not Active
 * 
 */
#define OLED_CMD_GPH_ACC        0x72 // Set Graphic Acceleration (followed by acceleration mode)
#define OLED_SET_GPH_ACC(GH, GV, GA)    (((!!(GH)) << 4) | ((!!(GV)) << 3) | ((GA) & 0x07))

/**
 * Graphic Acceleration Frame Interval
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_ACC_FRAME
 * - 2nd Byte: b[FI:8]
 * 
 */
#define OLED_CMD_ACC_FRAME      0x73 // Set Graphic Acceleration Frame Interval (followed by frame interval)
#define OLED_SET_ACC_FRAME(FI)  ((FI) & 0xFF)

/**
 * Graphic Acceleration Row Start/End
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_ACC_ROW
 * - 2nd Byte: SR b[SR:7]
 * - 3rd Byte: ER b[ER:7]
 * 
 */
#define OLED_CMD_ACC_ROW        0x74 // Set Graphic Acceleration Row Start/End (followed by row start and row end)
#define OLED_SET_ACC_ROWSTART(SR)    ((SR) & 0x7F)
#define OLED_SET_ACC_ROWEND(ER)      ((ER) & 0x7F)

#define OLED_CMD_BW_INV_ON      0x79 // Black and White Inversion ON
#define OLED_CMD_BW_INV_OFF     0x78 // Black and White Inversion OFF

#define OLED_CMD_OVERLAP_H      0xB1 // High Overlap
#define OLED_CMD_OVERLAP_L      0xB0 // Low Overlap

#define OLED_CMD_ENDIAN_RGB     0xB2 // Endian RGB (invalid in mono mode)
#define OLED_CMD_ENDIAN_BGR     0xB3 // Endian BGR (invalid in mono mode)

/**
 * Segment Swap
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_SEG_SWAP
 * - 2nd Byte: b0000000[SE:1]
 * 
 * SE = 1: EVEN/ODD Segment Swap On
 * SE = 0: EVEN/ODD Segment Swap Off
 * 
 */
#define OLED_CMD_SEG_SWAP       0xB4 // EVEN/ODD Segment Swap (followed by segment swap)
#define OLED_SET_SEG_SWAP(SE)   ((!!(SE)) & 0x01)

/**
 * Voltage and Current
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_VOL_CUR
 * - 2nd Byte: b[CE:1, IRE:1]000[DAC:3]
 * 
 * CE = 1: Voltage = DAC Setting = (0.72 + 0x02 * DAC) * VHA
 * CE = 0: Voltage = VHC
 * 
 * IRE = 1: Current is controlled by this register
 * IRE = 0: Current is external current source
 * 
 */
#define OLED_CMD_VOL_CUR        0xB5 // Set Voltage and Current (followed by voltage and current setting)
#define OLED_SET_VOL_CUR(CE, IRE, DAC)  (((!!(CE)) << 7) | ((!!(IRE)) << 6) | ((DAC) & 0x07))

/**
 * Brightness Decrease
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_BRIGHT_DEC
 * - 2nd Byte: b0[BR1:3]0[BR0:3]
 * - 3rd Byte: b0[BR3:3]0[BR2:3]
 * 
 */
#define OLED_CMD_BRIGHT_DEC     0xB7 // Set Brightness Decrease for Compensation (followed by brightness decrease)
#define OLED_SET_BRIGHT_10(BR1, BR0)    ((((BR1) & 0x07) << 4) | ((BR0) & 0x07))
#define OLED_SET_BRIGHT_32(BR3, BR2)    ((((BR3) & 0x07) << 4) | ((BR2) & 0x07))

/**
 * Brightness Control
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_BRIGHT
 * - 2nd Byte: b0[R:7] (Brightness R)
 * - 3rd Byte: b0[G:7] (Brightness G)
 * - 4th Byte: b0[G:7] (Brightness B)
 * 
 */
#define OLED_CMD_BRIGHT         0xB8 // Brightness Control (followed by brightness R, G, B)
#define OLED_SET_BRIGHT_R(R)   ((R) & 0x7F)
#define OLED_SET_BRIGHT_G(G)   ((G) & 0x7F)
#define OLED_SET_BRIGHT_B(B)   ((B) & 0x7F)

/**
 * Compensation Threshold
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_CMPSTN_THR
 * - 2nd Byte: b[THR1:8]
 * - 3rd Byte: b[THR2:8]
 * - 4th Byte: b[THR3:8]
 * - 5th Byte: b[THR4:8]
 * 
 * TH1~TH4 must corespond to: 0 < TH1 < TH2 < TH3 < TH4 < 128
 * 
 */
#define OLED_CMD_CMPSTN_THR     0xBA // Compensation Threshold (followed by threshold 1~4)
#define OLED_SET_CMPSTN_THR1(TH1)   ((TH1) & 0xFF)
#define OLED_SET_CMPSTN_THR2(TH2)   ((TH2) & 0xFF)
#define OLED_SET_CMPSTN_THR3(TH3)   ((TH3) & 0xFF)
#define OLED_SET_CMPSTN_THR4(TH4)   ((TH4) & 0xFF)

/**
 * Compensation Pulse Threshold
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_CMPSTN_PUL_THR
 * - 2nd Byte: b0[CPT:8] (Compensation Pulse Threshold)
 * 
 */
#define OLED_CMD_CMPSTN_PUL_THR 0xBB // Compensation Pulse Threshold (followed by pulse threshold)
#define OLED_SET_CMPSTN_PUL_THR(CPT)    ((CPT) & 0xFF)

/**
 * PWM Decrease
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_PWM_DEC
 * - 2nd Byte: b0000[PE4:1, PE3:1, PE2:1, PE1:1]
 * 
 * PE1 = 1: Enable Case 1 compensation by PWD table
 * PE2 = 1: Enable Case 2 compensation by PWD table
 * PE3 = 1: Enable Pulse Threshold compensation
 * PE4 = 1: Enable Brightness Compensation
 * 
 */
#define OLED_CMD_PWM_DEC        0xB6 // PWM Decrease (followed by PWM decrease setting)
#define OLED_SET_PWM_DEC(PE4, PE3, PE2, PE1)    (((!!(PE4)) << 3) | ((!!(PE3)) << 2) | ((!!(PE2)) << 1) | (!!(PE1)))

/**
 * PWM Decrease Table
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_PWM_DEC_TBL
 * - 2nd Byte: b[PWD1:4, PWD0:4]
 * - 3rd Byte: b[PWD3:4, PWD2:4]
 * - 4th Byte: b0000[PWD4:4]
 *
 * PWN_D(PWM Decrease):
 * - PWN_D[3:0]: Sets the PWM decrease value (0 to 15), PWD Table Decrease Pulse Width = PWN_D * PWMCLK
 * 
 */
#define OLED_CMD_PWM_DEC_TBL    0xB9 // PWM Decrease Table (followed by PWM decrease table)
#define OLED_SET_PWM_DEC_TBL_10(PWD1, PWD0) ((((PWD1) & 0x0F) << 4) | ((PWD0) & 0x0F))
#define OLED_SET_PWM_DEC_TBL_32(PWD3, PWD2) ((((PWD3) & 0x0F) << 4) | ((PWD2) & 0x0F))
#define OLED_SET_PWM_DEC_TBL_4(PWD4)    ((PWD4) & 0x0F)

#define OLED_CMD_CATH_SCAN_L    0xBC // Cathode Scan From COM0 to COMn
#define OLED_CMD_CATH_SCAN_R    0xBD // Cathode Scan From COMn to COM0

/**
 * Anode Trim
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_ANODE_TRIM
 * - 2nd Byte: b0000[TRIM:4] (Anode Trim)
 * 
 */
#define OLED_CMD_ANODE_TRIM     0xBE // Anode Trim (followed by trim value)
#define OLED_SET_ANODE_TRIM(TRIM)   ((TRIM) & 0x0F)

/**
 * Pre-Charge Period (Default Value = 0x0A)
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_PRECHARGE_PER
 * - 2nd Byte: b0000[PR:4]
 * 
 */
#define OLED_CMD_PRECHG_PRD     0xC8 // Pre-charge Period
#define OLED_SET_PRECHG_PRD(PR) ((PR) & 0x0F)

/**
 * Pre-Charge Current Command (Default Value = 0x03)
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_PRECHG_CUR
 * - 2nd Byte: b000000[PC:2]
 * 
 * PC (Pre-Charge Current):
 * - 00: Iseg0F X 2 = 62.5 μA
 * - 01: Iseg1F X 2 = 125 μA
 * - 10: Iseg3F X 2 = 250 μA
 * - 11: Iseg7F X 2 = 500 μA
 * 
 */
#define OLED_CMD_PRECHG_CUR     0xD0 // Pre-Charge Current Command
#define OLED_SET_PRECHG_CUR(PC) ((PC) & 0x03)

#define OLED_CMD_ALL_ZERO_ON    0xCD // Enable All Zero Blank Mode
#define OLED_CMD_ALL_ZERO_OFF   0xCC // Disable All Zero Blank Mode

/**
 * COM Pulse Width Command (Default Value = 0x06)
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_COM_PULSE_WIDTH
 * - 2nd Byte: b[COM_W:8] (Least significant 8 bits of the width)
 * - 3rd Byte: b0000000[COM_W:1] (Most significant bit of the width)
 * 
 * COM Pulse Width (PWMCLK) = COM_W[8:0] + 65
 */
#define OLED_CMD_COM_PLS_WIDTH  0xD8 // Set COM Pulse Width
#define OLED_SET_COM_PLS_WIDTH_BYTE_1(W)    ((W) & 0xFF)
#define OLED_SET_COM_PLS_WIDTH_BYTE_2(W)    (((W) >> 8) & 0x01)

/**
 * COM Number Command
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_COM_NUMBER
 * - 2nd Byte: b0[COM_N:7] (Number of rows to display)
 *
 * - COM_N[6:0] = 1 -> 32 rows (Color Mode)
 * - COM_N[6:0] = 1 -> 96 rows (Mono Mode)
 */
#define OLED_CMD_COM_NUM        0xE0 // Set COM Number
#define OLED_SET_COM_NUM(COM_N) ((COM_N) & 0x7F)

/**
 * Display Row & Column Start
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_ROW_START
 * - 2nd Byte: b0[data:7] (Start)
 * 
 */
#define OLED_CMD_ROW_START      0xE1 // Set Display Row Start
#define OLED_SET_ROW_START(DR)  ((DR) & 0x7F)
#define OLED_CMD_COL_START      0xE2 // Set Display Column Start
#define OLED_SET_COL_START(DC)  ((DC) & 0x7F)

/**
 * COM Output Delay Mode
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_COM_OUT_DELAY
 * - 2nd Byte: b000000[D:2]
 * 
 * D (Delay Time, T_PWNCLK * 4):
 * - 00: 0
 * - 01: 1
 * - 10: 2
 * - 11: 3
 * 
 */
#define OLED_CMD_COM_OUT_DELAY  0xE3 // Set COM Output Delay Mode
#define OLED_SET_COM_OUT_DELAY(D)   ((D) & 0x03)

/**
 * Black/Blank Period
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_BLK_PER
 * - 2nd Byte: b[BLACK:3, BLANK:5]
 * 
 */
#define OLED_CMD_BLK_PER        0xE5 // Set Black/Blank Period
#define OLED_SET_BLK_PER(BLACK, BLANK)  ((((BLACK) & 0x07) << 5) | ((BLANK) & 0x1F))

#define OLED_CMD_DUMMY_SCAN_ON  0xE7 // Enable Dummy Scan
#define OLED_CMD_DUMMY_SCAN_OFF 0xE6 // Disable Dummy Scan

/**
 * Test MOD
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_TEST_MOD
 * - 2nd Byte: OLED_SET_TEST_BYTE1
 * - 3rd Byte: OLED_SET_TEST_BYTE2
 * - 4th Byte: b0000000[TM:1]
 * 
 * TM (Test MOD Enable):
 * - 0: Disable
 * - 1: Enable
 * 
 */
#define OLED_CMD_TEST_MOD       0xC4 // Enable Test MOD
#define OLED_SET_TEST_BYTE1     0x55
#define OLED_SET_TEST_BYTE2     0xAA
#define OLED_SET_TEST_MOD(TM)   ((!!TM) & 0x01)

/**
 * Driver Output Test Mode
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_DRV_TEST
 * - 2nd Byte: b[T7:1, T6:1, T5:1, T4:1, T3:1, T2:1, T1:1, T0:1]
 * 
 * [T2, T1, T0],    Segment Driver, Common Driver,  Bias Circuit:
 * - 000:           Normal,         Normal,         Enabled
 * - 001:           All Low,        All Low,        Enabled
 * - 010:           All High,       All High,       Enabled
 * - 011:           High-Impedance, Normal,         Disabled
 * - 100:           High-Impedance, Serial Input,   Enabled
 * - 101:           Serial Input,   Serial Input,   Enabled
 * - 110:           Serial Input,   Serial Input,   Enabled
 * - 111:           High-Impedance, All High(H-Overlap) or All Low(L-Overlap), Disabled
 * 
 * T7,  Segment Driver:
 * - 0: Normal
 * - 1: High-Impedance
 * 
 * T6,  Common Driver:
 * - 0: Normal
 * - 1: All High(H-Overlap) or All Low(L-Overlap)
 * 
 * T5,  Bias Circuit:
 * - 0: Enabled
 * - 1: Disabled
 * 
 * T4 = 1: D7-D0 will be set to output mode and RAM is filled with "TESTRGB data"
 * 
 * T3 = 1: RAM is filled with "TESTRGB data"
 */
#define OLED_CMD_DRV_TEST       0xC0 // Set Driver Output Test Mode
#define OLED_SET_DRV_TEST(T7, T6, T5, T4, T3, T2, T1, T0)   (((!!(T7)) << 7) | ((!!(T6)) << 6) | (!!(T5) << 5) | (!!(T4) << 4) | (!!(T3) << 3) | (!!(T2) << 2) | (!!(T1) << 1) | (!!(T0)))

/**
 * Test RGB Data
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_TEST_RGB_DATA
 * - 2nd Byte: b[D:8]
 * 
 */
#define OLED_CMD_TEST_RGB_DATA  0xC1 // Set Test RGB Data
#define OLED_SET_TEST_RGB_DATA(D) ((D) & 0xFF)

/**
 * Clock Output Test Mode
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_CLK_TEST
 * - 2nd Byte: b0000000[CO:1]
 * 
 * CO = 1: INT pin will output clock signal
 */
#define OLED_CMD_CLK_TEST       0xC5 // Set Clock Output Test Mode
#define OLED_SET_CLK_TEST(CO)   ((!!(CO)) & 0x01)

/**
 * Parameter Read Test Mode
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_PRM_TEST
 * - 2nd Byte(Output): b[data:8]
 * - nth Byte(Output): b[data:8]
 * 
 */
#define OLED_CMD_PRM_TEST       0xCE // Set Parameter Read Test Mode

/**
 * Read Scan COM Order
 * 
 * Byte Information:
 * - 1st Byte: OLED_CMD_SCAN_COM
 * - 2nd Byte(Output): b[data:8]
 * 
 */
#define OLED_CMD_SCAN_COM       0xCF // Read Scan COM Order

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OLED_PANEL_CMDS_H__