#ifndef SC_H
#define SC_H

#include <stdint.h>

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

//EPC addresses

static const uint32_t EPC_PRH0_BASE_ADDRESS = 0x43000000;

static const uint32_t SERIAL_MSB_OFFSET                        = 0;
static const uint32_t SERIAL_LSB_OFFSET                        = 4;
static const uint32_t MULTI_PURPOSE_OFFSET                     = 8;
static const uint32_t PLL_READBACK_REGISTER_OFFSET             = 12;
static const uint32_t PLL_CONFIG_OFFSET                        = 16;
static const uint32_t PLL_PROG_REGISTER_OFFSET                 = 20;
static const uint32_t SPI_OFFSET                               = 24;
static const uint32_t IOB_CNTR_1_OFFSET                        = 28;
static const uint32_t IOB_CNTR_2_OFFSET                        = 32;
static const uint32_t IOB_CNTR_3_OFFSET                        = 36;
static const uint32_t IOB_CNTR_4_OFFSET                        = 40;
static const uint32_t CLOCK_FANOUT_CONTROL_OFFSET              = 44;
static const uint32_t CMD_PROT_ENGINE_OFFSET                   = 48;
static const uint32_t ASIC_JTAG_ENGINE_1_OFFSET                = 52;
static const uint32_t ASIC_JTAG_ENGINE_2_OFFSET                = 56;
static const uint32_t ASIC_JTAG_ENGINE_3_OFFSET                = 60;
static const uint32_t ASIC_JTAG_ENGINE_4_OFFSET                = 64;
static const uint32_t ETHERNET_ENGINE_1_CONFIG_OFFSET          = 68;
static const uint32_t ETHERNET_ENGINE_2_CONFIG_OFFSET          = 72;
static const uint32_t ETHERNET_ENGINE_3_CONFIG_OFFSET          = 76;
static const uint32_t ETHERNET_ENGINE_4_CONFIG_OFFSET          = 80;
static const uint32_t AURORA_RX_CONTROL_OFFSET                 = 84;
static const uint32_t AURORA_RX_READBACK_1_OFFSET              = 88;
static const uint32_t AURORA_RX_READBACK_2_OFFSET              = 92;
static const uint32_t ETHERNET_READBACK_1_OFFSET               = 96;
static const uint32_t ETHERNET_READBACK_2_OFFSET               = 100;
static const uint32_t DATARECV_TO_ETH0_REG_OFFSET              = 104;
static const uint32_t ETHERNET_RESET_REGISTER_OFFSET           = 108;
static const uint32_t XADC_TEMP_OUT_OFFSET                     = 112;
static const uint32_t IOB_TEMP_OUT_OFFSET                      = 116;
static const uint32_t PLL_DRP_DATA_REGISTER_OFFSET             = 120;
static const uint32_t DDR3_READBACK_REGISTER_OFFSET            = 124;
static const uint32_t BUILD_DATE_READBACK_REGISTER_OFFSET      = 128;
static const uint32_t BUILD_TIME_REV_READBACK_REGISTER_OFFSET  = 132;
static const uint32_t ICAP_DATA_REGISTER_OFFSET                = 136;
static const uint32_t ASIC_JTAG_CONTROL_REGISTER_OFFSET        = 140;
static const uint32_t ASIC_JTAG_STATUS_REGISTER_OFFSET         = 144;
static const uint32_t SIB_UART_DATA_OFFSET                     = 148;
static const uint32_t SIB_UART_CTRL_OFFSET                     = 152;
static const uint32_t LINKID_OFFSET                            = 156;
static const uint32_t CHANNEL_FAILIURE0_OFFSET                 = 160;
static const uint32_t CHANNEL_FAILIURE1_OFFSET                 = 164;
static const uint32_t ASIC_TEMPERATURE_OFFSET                  = 168;
static const uint32_t FIFO_TEST_OFFSET                         = 172;
static const uint32_t SIB_RECEIVE_REGISTER                     = 176;
static const uint32_t SINGLE_CYCLE_REGISTER                    = 180;
static const uint32_t DET_SPECIFIC_DATA_REGISTER               = 184;
static const uint32_t DATA_MONITOR_SEL_REGISTER                = 188;
static const uint32_t DATA_MONITOR_STATUS_REGISTER             = 192;
static const uint32_t DATA_MONITOR_READ_REGISTER               = 196;
static const uint32_t DATA_MONITOR_TRAINID_REGISTER            = 200;

static const uint8_t c_addrSPI            = 0x0b;
static const uint8_t c_cmdSPI_write       = 0x00;
static const uint8_t c_SPI_addrHV         = 0x01;
static const uint8_t c_SPI_addrDAISYCHAIN = 0x02;
static const uint8_t c_SPI_addrDAC        = 0x04;

static const uint8_t c_cmdIOBCLREngine_startXfelClk = 0xA0;
static const uint8_t c_cmdIOBCLREngine_stopXfelClk  = 0xA1;

static const uint8_t c_cmdIOBCNTREngine_startsend    = 0xA0;
static const uint8_t c_cmdIOBCNTREngine_startread    = 0xA1;
static const uint8_t c_cmdIOBCNTREngine_en_perm_res  = 0xB0;
static const uint8_t c_cmdIOBCNTREngine_dis_perm_res = 0xB1;

static const uint8_t c_cmdRXAURORAEngine_en_pwr_dwn  = 0xA1;
static const uint8_t c_cmdRXAURORAEngine_dis_pwr_dwn = 0xA0;

static const uint8_t c_IOBoardCmd_write = 0b00101001;
static const uint8_t c_IOBoardCmd_read  = 0b00101000;

static const uint16_t c_IOBoardConfRegAddr_serial_lsb  = 0x0000;
static const uint16_t c_IOBoardConfRegAddr_serial_msb  = 0x0004;
static const uint16_t c_IOBoardConfRegAddr_build_date  = 0x0008;
static const uint16_t c_IOBoardConfRegAddr_build_stat  = 0x000C;
static const uint16_t c_IOBoardConfRegAddr_board_type  = 0x0010;

static const uint16_t c_IOBoardConfRegAddr_lmk01010_stats = 0x0100; //000000 busy ack dev_sel valid en
static const uint16_t c_IOBoardConfRegAddr_lmk01010_data  = 0x0104; //

static const uint16_t c_IOBoardConfRegAddr_prb_stat          = 0x0200;
static const uint16_t c_IOBoardConfRegAddr_prb_gdps_dly      = 0x0204;
static const uint16_t c_IOBoardConfRegAddr_prb_power_on_dly  = 0x0208;
static const uint16_t c_IOBoardConfRegAddr_prb_power_on_time = 0x020C;
static const uint16_t c_IOBoardConfRegAddr_prb_num_prbs      = 0x0210;
static const uint16_t c_IOBoardConfRegAddr_prb_ctrl_sw_supplies_always_on = 0x0214;
static const uint16_t c_IOBoardConfRegAddr_prb_manual_config_lsb = 0x0218;
static const uint16_t c_IOBoardConfRegAddr_prb_manual_config_msb = 0x021C;

static const uint16_t c_IOBoardConfRegAddr_fetdrv_en           = 0x0300;
static const uint16_t c_IOBoardConfRegAddr_fetdrv_dvgate_on    = 0x0304;
static const uint16_t c_IOBoardConfRegAddr_fetdrv_dvsource_on  = 0x0308;
static const uint16_t c_IOBoardConfRegAddr_fetdrv_dvsss_on     = 0x030C;
static const uint16_t c_IOBoardConfRegAddr_fetdrv_dvgate_off   = 0x0310;
static const uint16_t c_IOBoardConfRegAddr_fetdrv_dvsource_off = 0x0314;
static const uint16_t c_IOBoardConfRegAddr_fetdrv_dvsss_off    = 0x0318;

static const uint16_t c_IOBoardConfRegAddr_preclr_dly     = 0x0400;
static const uint16_t c_IOBoardConfRegAddr_clron_ofs      = 0x0404;
static const uint16_t c_IOBoardConfRegAddr_clroff_ofs     = 0x0408;
static const uint16_t c_IOBoardConfRegAddr_clrgateon_ofs  = 0x040C;
static const uint16_t c_IOBoardConfRegAddr_clrgateoff_ofs = 0x0410;
static const uint16_t c_IOBoardConfRegAddr_clr_period     = 0x0414;
static const uint16_t c_IOBoardConfRegAddr_clr_duty       = 0x0418;
static const uint16_t c_IOBoardConfRegAddr_clrdis_duty    = 0x041C;
static const uint16_t c_IOBoardConfRegAddr_clrdis_en_dly  = 0x0420;
static const uint16_t c_IOBoardConfRegAddr_clrdis_ctrl_en = 0x0424;

static const uint16_t c_IOBoardConfRegAddr_aurora_swing    = 0x0500;
static const uint16_t c_IOBoardConfRegAddr_aurora_emph     = 0x0504;
static const uint16_t c_IOBoardConfRegAddr_aurora_status   = 0x0510;
static const uint16_t c_IOBoardConfRegAddr_aurora_reset    = 0x0514;

static const uint16_t c_IOBoardConfRegAddr_enable_asic_ro  = 0x0600;
static const uint16_t c_IOBoardConfRegAddr_send_dummy_data = 0x0604;
static const uint16_t c_IOBoardConfRegAddr_asic_reset      = 0x0608;
static const uint16_t c_IOBoardConfRegAddr_asic_status     = 0x060C;
static const uint16_t c_IOBoardConfRegAddr_asic_delay      = 0x0610;
static const uint16_t c_IOBoardConfRegAddr_pptdata_delay   = 0x0614;

static const uint16_t c_IOBoardConfRegAddr_self_trigger       = 0x0700;
static const uint16_t c_IOBoardConfRegAddr_burst_length       = 0x0704;
static const uint16_t c_IOBoardConfRegAddr_iprog_length       = 0x0708;
static const uint16_t c_IOBoardConfRegAddr_burst_cycle_length = 0x070C;
static const uint16_t c_IOBoardConfRegAddr_iprog_cycle_length = 0x0710;
static const uint16_t c_IOBoardConfRegAddr_refpulse_length    = 0x0714;
static const uint16_t c_IOBoardConfRegAddr_sysfsm_status      = 0x0718;

static const uint16_t c_IOBoardConfRegAddr_dummy = 0xfffc;


#endif
