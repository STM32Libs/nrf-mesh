
#include "nrf24l01p.h"

Nrf24l01p::Nrf24l01p(Serial *ps,PinName ce, PinName csn, PinName sck, PinName mosi, PinName miso):
                spi(mosi,miso,sck),
                csn_pin(csn),
                ce_pin(ce),
                mode(nrf::Mode::Uninitialised),
                pr(ps)
{
    //spi default @10 Mbps
    ce_pin_lowDisable();
    csn_pin_highClear();
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

//--------------------- Level 3 - Modes ----------------------------
void Nrf24l01p::setMode(nrf::Mode m)
{
    switch(m)
    {
        case nrf::Mode::Standby :
        {
            setbit(nrf::reg::CONFIG,nrf::bit::config::PWR_UP);
            wait_us(nrf::delay::Tpd2stby_us);
            mode = nrf::Mode::Standby;
        }
        break;
        case nrf::Mode::PowerDown :
        {
            clearbit(nrf::reg::CONFIG,nrf::bit::config::PWR_UP);
            mode = nrf::Mode::PowerDown;
        }
        break;
        case nrf::Mode::Rx :
        {
            if(mode == nrf::Mode::PowerDown)
            {
                setMode(nrf::Mode::Standby);
            }
            setbit(nrf::reg::CONFIG,nrf::bit::config::PRIM_RX);
            mode = nrf::Mode::Rx;
        }
        break;
        case nrf::Mode::Tx :
        {
            if(mode == nrf::Mode::PowerDown)
            {
                setMode(nrf::Mode::Standby);
            }
            clearbit(nrf::reg::CONFIG,nrf::bit::config::neg_PRIM_RX);//Rx cleared => Tx mode
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
void Nrf24l01p::setDataRate(nrf::datarate dr)
{
    uint8_t rf_setup_reg = readRegister(nrf::reg::RF_SETUP);
    rf_setup_reg &= ~ nrf::bit::rf_setup::RF_DR_MASK;
    switch(dr)
    {
        case nrf::datarate::d_2Mbps :
        {
            rf_setup_reg |= nrf::bit::rf_setup::RF_DR_2MBPS;
        }
        break;
        case nrf::datarate::d_1Mbps :
        {
            rf_setup_reg |= nrf::bit::rf_setup::RF_DR_1MBPS;
        }
        break;
        case nrf::datarate::d_250Kbps :
        {
            rf_setup_reg |= nrf::bit::rf_setup::RF_DR_250KBPS;
        }
        break;
        default:
        {
            //TODO notify  error
        }
    }

    writeRegister(nrf::reg::RF_SETUP,rf_setup_reg);
}

void Nrf24l01p::selectChannel(uint8_t chan)
{

}

uint8_t Nrf24l01p::getChannel()
{

}

void Nrf24l01p::setTxAddress()
{

}

void Nrf24l01p::setRxAddress()
{

}

void Nrf24l01p::setCrcConfig(nrf::crc c)
{
    uint8_t config = readRegister(nrf::reg::CONFIG);
    if(c == nrf::crc::NoCrc)
    {
        config &= ~nrf::bit::config::EN_CRC;
    }
    else
    {
        if(c == nrf::crc::TwoBytes)
        {
            config |= nrf::bit::config::CRCO;
        }
        else
        {
            config &= ~nrf::bit::config::CRCO;
        }
        config |= nrf::bit::config::EN_CRC;
    }

    writeRegister(nrf::reg::CONFIG,config);
}

void Nrf24l01p::disableAutoAcknowledge()
{
    writeRegister(nrf::reg::EN_AA,0);//disable Auto Acknowledge for all pipes
}

void Nrf24l01p::enableAutoAcknowledge(uint8_t bits)
{
    writeRegister(nrf::reg::EN_AA,bits);
}

void Nrf24l01p::disableRxPipes()
{
    writeRegister(nrf::reg::EN_RXADDR,0);
}

void Nrf24l01p::enableRxPipes(uint8_t bits)
{
    writeRegister(nrf::reg::EN_RXADDR,bits);
}

void Nrf24l01p::disableRetransmission()
{
    writeRegister(nrf::reg::SETUP_RETR, 0);
}

void Nrf24l01p::setPipeWidth(uint8_t pipe, uint8_t width)
{
    //here a mask on width does not make sense but a test yes as 32 is valid but 33 not
    writeRegister(nrf::reg::RX_PW_P0 + pipe, width);
}
//--------------------- Level 2 - Status ----------------------------
uint8_t Nrf24l01p::getRxPayloadWidth()
{
    return readRegister(nrf::cmd::R_RX_PL_WID);
}
//--------------------- Level 2 - Actions ----------------------------
void Nrf24l01p::flushRX()
{
    command(nrf::cmd::FLUSH_RX);
}

//--------------------- Level 3 - Communication ----------------------------
void Nrf24l01p::receive(uint8_t *payload, uint8_t size)
{

}

void Nrf24l01p::wait_transmit()
{
	uint8_t status;
    int cycles = 0;
	do
	{
		status = readStatus();
		cycles++;
        wait_us(nrf::delay::Poll_Tx_10_us);
	}while(     ((status & nrf::bit::status::TX_DS) == 0)
                &
                (cycles<255)
            );
}

void Nrf24l01p::start_transmission(uint8_t *payload, uint8_t size)
{
    //Assert Data Sent before new transmission to poll TX status
    //no need to use setbit with status, as two read, and others act on write one only
    writeRegister(nrf::reg::STATUS,nrf::bit::status::TX_DS);

    if(mode != nrf::Mode::Tx)
    {
        setMode(nrf::Mode::Tx);
    }

    //Fill the payload
    writeBuffer(nrf::cmd::W_TX_PLOAD,payload,size);

    ce_pin_highEnable();
    wait_us(nrf::delay::Tx_Pulse_10_us);
    ce_pin_lowDisable();
}

void Nrf24l01p::transmit_Down(uint8_t *payload, uint8_t size)
{
    start_transmission(payload,size);
    wait_transmit();
    setMode(nrf::Mode::PowerDown);
}

void Nrf24l01p::transmit_Rx(uint8_t *payload, uint8_t size)
{
    start_transmission(payload,size);
    wait_transmit();
    setMode(nrf::Mode::Rx);
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
    pr->printf("STATUS 0x%02x : ",status);
    if(status & nrf::bit::status::TX_FULL)
    {
        pr->printf("Tx FIFO full - ");
    }
    else
    {
        pr->printf("Tx FIFO Free - ");
    }
    if((status & nrf::bit::status::RX_P_NO) == nrf::bit::status::RX_P_NO)
    {
        pr->printf("Rx FIFO Empty - ");
    }
    else
    {
        uint8_t pipe_nb = (status & nrf::bit::status::RX_P_NO)>>1;
        pr->printf("Rx Available @ %d - ",pipe_nb);
    }
    bool interrupt_pending = false;
    if(status & nrf::bit::status::MAX_RT)
    {
        interrupt_pending = true;
        pr->printf("<INT> MAX_RT  - ");
    }
    if(status & nrf::bit::status::TX_DS)
    {
        interrupt_pending = true;
        pr->printf("<INT> Tx_DS  - ");
    }
    if(status & nrf::bit::status::RX_DR)
    {
        interrupt_pending = true;
        pr->printf("<INT> RX_DR  - ");
    }
    if(interrupt_pending == false)
    {
        pr->printf("No <INT>");
    }
    pr->printf("\n");
}

void Nrf24l01p::print_config()
{
    uint8_t config = readRegister(nrf::reg::CONFIG);
    pr->printf("CONFIG 0x%02x : ",config);
    if(config & nrf::bit::config::PRIM_RX)
    {
        pr->printf("Rx Mode - ");
    }
    else
    {
        pr->printf("Tx Mode - ");
    }
    if(config & nrf::bit::config::PWR_UP)
    {
        pr->printf("Power Up - ");
    }
    else
    {
        pr->printf("Power Down - ");
    }
    if(config & nrf::bit::config::EN_CRC)
    {
        pr->printf("CRC Enabled : ");
        if(config & nrf::bit::config::CRCO)
        {
            pr->printf("2 Bytes - ");
        }
        else
        {
            pr->printf("1 Byte - ");
        }
    }
    else
    {
        pr->printf("CRC Disabled - ");
    }
    bool interrupt_enabled = false;
    if((config & nrf::bit::config::MASK_MAX_RT) == 0)
    {
        interrupt_enabled = true;
        pr->printf("[INT] MAX_RT  - ");
    }
    if((config & nrf::bit::config::MASK_TX_DS) == 0)
    {
        interrupt_enabled = true;
        pr->printf("[INT] TX_DS  - ");
    }
    if((config & nrf::bit::config::MASK_RX_DR) == 0)
    {
        interrupt_enabled = true;
        pr->printf("[INT] RX_DR  - ");
    }
    if(!interrupt_enabled)
    {
        pr->printf("No Interrupt Enabled");
    }
    pr->printf("\n");
    
    
}

void Nrf24l01p::print_rf_setup()
{
    uint8_t rf_setup = readRegister(nrf::reg::RF_SETUP);
    pr->printf("RF_SETUP 0x%02x : ",rf_setup);
    uint8_t pwr = rf_setup & nrf::bit::rf_setup::RF_PWR_MASK;
    switch(pwr)
    {
        case nrf::bit::rf_setup::RF_PWR_MIN_18DBM :
        {
            pr->printf("PWR -18dBm - ");
        }break;
        case nrf::bit::rf_setup::RF_PWR_MIN_12DBM :
        {
            pr->printf("PWR -12dBm - ");
        }break;
        case nrf::bit::rf_setup::RF_PWR_MIN_6DBM :
        {
            pr->printf("PWR -6dBm - ");
        }break;
        case nrf::bit::rf_setup::RF_PWR_0DBM :
        {
            pr->printf("PWR 0dBm - ");
        }break;
    };
    if(rf_setup & nrf::bit::rf_setup::RF_DR_LOW_BIT)
    {
        pr->printf("DR 250Kbps - ");
    }
    else
    {
        if(rf_setup & nrf::bit::rf_setup::RF_DR_HIGH_BIT)
        {
            pr->printf("DR 2Mbps - ");
        }
        else
        {
            pr->printf("DR 1Mbps - ");
        }
    }
    if(rf_setup & nrf::bit::rf_setup::PLL_LOCK)
    {
        pr->printf("PLL Lock Forced - ");
    }
    if(rf_setup & nrf::bit::rf_setup::CONT_WAVE)
    {
        pr->printf("Continuous Carrier");
    }
    pr->printf("\n");
}

void Nrf24l01p::print_fifo_status()
{
    int8_t fifo_status = readRegister(nrf::reg::FIFO_STATUS);
    pr->printf("FIFO_STATUS 0x%02x : ",fifo_status);
    if(fifo_status & nrf::bit::fifo_status::RX_EMPTY)
    {
        pr->printf("RX Empty - ");
    }
    else
    {
        pr->printf("Data in RX - ");
    }
    if(fifo_status & nrf::bit::fifo_status::RX_FULL)
    {
        pr->printf("RX Full - ");
    }
    else
    {
        pr->printf("Available RX - ");
    }
    if(fifo_status & nrf::bit::fifo_status::TX_EMPTY)
    {
        pr->printf("TX Empty - ");
    }
    else
    {
        pr->printf("Data in TX - ");
    }
    if(fifo_status & nrf::bit::fifo_status::TX_FULL)
    {
        pr->printf("TX Full - ");
    }
    else
    {
        pr->printf("Available TX - ");
    }
    if(fifo_status & nrf::bit::fifo_status::TX_REUSE)
    {
        pr->printf("TX Reuse");
    }
    pr->printf("\n");
}

void Nrf24l01p::dump_regs()
{
    for(uint8_t add=0;add<=0x09;add++)
    {
        uint8_t val = readRegister(add);
        pr->printf("0x%02x : 0x%02x\n",add,val);
    }
    for(uint8_t add=0x0A;add<=0x0B;add++)
    {
        uint8_t vals[5];
        readBuffer(add, vals,5);
        pr->printf("0x%02x : ",add);
        for(int i=0;i<5;i++)
        {
            pr->printf("0x%02x ",vals[i]);
        }
        pr->printf("\n");
    }
    for(uint8_t add=0x0C;add<=0x17;add++)
    {
        uint8_t val = readRegister(add);
        pr->printf("0x%02x : 0x%02x\n",add,val);
    }
    uint8_t add = 0x1C;
    uint8_t val = readRegister(add);
    pr->printf("0x%02x : 0x%02x\n",add,val);
}