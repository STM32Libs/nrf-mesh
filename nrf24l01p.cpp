
#include "nrf24l01p.h"

Serial   pr(PB_10, PB_11, 115200);


Nrf24l01p::Nrf24l01p(PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso):
                spi(mosi,miso,sck),
                csn_pin(csn),
                ce_pin(ce),
                mode(nrf::Mode::Uninitialised)
{

}

//--------------------- Level 0 ----------------------------
void Nrf24l01p::ce_pin_highEnable()
{
    ce_pin = 1;
    wait_us(nrf::delay::Tpece2csn_us);
}
void Nrf24l01p::ce_pin_lowDisable()
{
    ce_pin = 0;
}

void Nrf24l01p::csn_pin_lowSelect()
{
    csn_pin = 0;
}
void Nrf24l01p::csn_pin_highClear()
{
    csn_pin = 1;
}

//--------------------- Level 1 ----------------------------
void Nrf24l01p::command(uint8_t v_cmd)
{
    uint8_t last_ce = ce_pin;

    ce_pin_lowDisable();
    csn_pin_lowSelect();

    spi.write(v_cmd);

    csn_pin_highClear();
    if(last_ce)
    {
        ce_pin_highEnable();
    }

    wait_us(nrf::delay::Tpece2csn_us);

}

uint8_t Nrf24l01p::writeRegister(uint8_t reg,uint8_t val)
{
    uint8_t res = 0;
    uint8_t last_ce = ce_pin;

    ce_pin_lowDisable();
    csn_pin_lowSelect();

    res = spi.write(nrf::cmd::WRITE_REG | reg);
    spi.write(val);

    csn_pin_highClear();
    if(last_ce)
    {
        ce_pin_highEnable();
    }

    wait_us(nrf::delay::Tpece2csn_us);

    return res;
}

uint8_t Nrf24l01p::readRegister(uint8_t reg)
{
    csn_pin_lowSelect();

    spi.write(nrf::cmd::READ_REG | reg);
    uint8_t res_val = spi.write(nrf::cmd::NOP);

    csn_pin_highClear();

    return res_val;
}

uint8_t Nrf24l01p::readStatus()
{
    csn_pin_lowSelect();
    uint8_t res_val = spi.write(nrf::cmd::NOP);
    csn_pin_highClear();
    return res_val;
}

void Nrf24l01p::setbit(uint8_t reg, uint8_t bit)
{
    uint8_t val = readRegister(reg);
    val |= bit;
    writeRegister(reg,val);
}

void Nrf24l01p::clearbit(uint8_t reg, uint8_t bit)
{
    uint8_t val = readRegister(reg);
    val &= ~bit;
    writeRegister(reg,val);
}

void Nrf24l01p::writeBuffer(uint8_t add,uint8_t *buf,uint8_t size)
{
    uint8_t last_ce = ce_pin;

    ce_pin_lowDisable();
    csn_pin_lowSelect();

    spi.write(add);
    for(int i=0;i<size;i++)
    {
        spi.write(*buf++);
    }

    csn_pin_highClear();
    if(last_ce)
    {
        ce_pin_highEnable();
    }

    wait_us(nrf::delay::Tpece2csn_us);

}

void Nrf24l01p::readBuffer(uint8_t add,uint8_t *buf,uint8_t size)
{
    csn_pin_lowSelect();

    spi.write(add);
    for(int i=0;i<size;i++)
    {
        (*buf++) = spi.write(nrf::cmd::NOP);
    }

    csn_pin_highClear();

}

//--------------------- Level 2 - Modes ----------------------------
void Nrf24l01p::setMode(nrf::Mode m)
{
    switch(m)
    {
        case nrf::Mode::Standby :
        {
            setbit(nrf::reg::CONFIG,nrf::bit::CONFIG_PWR_UP);
            wait_us(nrf::delay::Tpd2stby_us);
            mode = nrf::Mode::Standby;
        }
        break;
        case nrf::Mode::PowerDown :
        {
            clearbit(nrf::reg::CONFIG,nrf::bit::CONFIG_PWR_UP);
            mode = nrf::Mode::PowerDown;
        }
        break;
        case nrf::Mode::Rx :
        {
            if(mode == nrf::Mode::PowerDown)
            {
                setMode(nrf::Mode::Standby);
            }
            setbit(nrf::reg::CONFIG,nrf::bit::CONFIG_PRIM_RX);
            mode = nrf::Mode::Rx;
        }
        break;
        case nrf::Mode::Tx :
        {
            if(mode == nrf::Mode::PowerDown)
            {
                setMode(nrf::Mode::Standby);
            }
            clearbit(nrf::reg::CONFIG,nrf::bit::CONFIG_neg_PRIM_RX);//Rx cleared => Tx mode
            mode = nrf::Mode::Tx;
        }
        break;
        default:
        {
            //? What to do ?
        }
        break;
        
    }
}
//--------------------- Level 2 - Config ----------------------------
void Nrf24l01p::set_DataRate(nrf::datarate dr)
{
    uint8_t rf_setup_reg = readRegister(nrf::reg::RF_SETUP);
    rf_setup_reg &= ~ nrf::bit::RF_SETUP_RF_DR_MASK;
    switch(dr)
    {
        case nrf::datarate::d_2Mbps :
        {
            rf_setup_reg |= nrf::bit::RF_SETUP_RF_DR_2MBPS;
        }
        break;
        case nrf::datarate::d_1Mbps :
        {
            rf_setup_reg |= nrf::bit::RF_SETUP_RF_DR_1MBPS;
        }
        break;
        case nrf::datarate::d_250Kbps :
        {
            rf_setup_reg |= nrf::bit::RF_SETUP_RF_DR_250KBPS;
        }
        break;
        default:
        {
            //TODO notify  error
        }
    }

    writeRegister(nrf::reg::RF_SETUP,rf_setup_reg);
}

void Nrf24l01p::select_Channel(uint8_t chan)
{

}

uint8_t Nrf24l01p::get_Channel()
{

}

void Nrf24l01p::set_TxAddress()
{

}

void Nrf24l01p::set_RxAddress()
{

}

void Nrf24l01p::set_CrcConfig(nrf::crc c)
{
    uint8_t config = readRegister(nrf::reg::CONFIG);
    if(c == nrf::crc::NoCrc)
    {
        config &= ~nrf::bit::CONFIG_EN_CRC;
    }
    else
    {
        if(c == nrf::crc::TwoBytes)
        {
            config |= nrf::bit::CONFIG_CRCO;
        }
        else
        {
            config &= ~nrf::bit::CONFIG_CRCO;
        }
        config |= nrf::bit::CONFIG_EN_CRC;
    }

    writeRegister(nrf::reg::CONFIG,config);
}

//--------------------- Level 3 - Info ----------------------------
void Nrf24l01p::print_info()
{
    print_status();
    print_config();
    print_rf_setup();
    print_fifo_status();
}

void Nrf24l01p::print_status()
{
    uint8_t status = readStatus();
    pr.printf("STATUS:0x%02x - ",status);
    if(status & nrf::bit::STATUS_TX_FULL)
    {
        pr.printf("Tx FIFO full - ");
    }
    else
    {
        pr.printf("Tx FIFO Free - ");
    }
    if((status & nrf::bit::STATUS_RX_P_NO) == nrf::bit::STATUS_RX_P_NO)
    {
        pr.printf("Rx FIFO Empty - ");
    }
    else
    {
        uint8_t pipe_nb = (status & nrf::bit::STATUS_RX_P_NO)>>1;
        pr.printf("Rx Available @ %d - ",pipe_nb);
    }
    bool interrupt_pending = false;
    if(status & nrf::bit::STATUS_MAX_RT)
    {
        interrupt_pending = true;
        pr.printf("<INT> MAX_RT  - ");
    }
    if(status & nrf::bit::STATUS_TX_DS)
    {
        interrupt_pending = true;
        pr.printf("<INT> Tx_DS  - ");
    }
    if(status & nrf::bit::STATUS_RX_DR)
    {
        interrupt_pending = true;
        pr.printf("<INT> RX_DR  - ");
    }
    if(interrupt_pending == false)
    {
        pr.printf("No <INT>");
    }
    pr.printf("\n");
}

void Nrf24l01p::print_config()
{

}

void Nrf24l01p::print_rf_setup()
{

}

void Nrf24l01p::print_fifo_status()
{

}

