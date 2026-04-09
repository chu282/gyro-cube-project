#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "adc.h"

uint32_t adc_fifo_out = 0;

void init_adc() {
    adc_init();
    adc_gpio_init(45);
    adc_select_input(5);
    adc_run(true);

    gpio_set_dir(45, GPIO_IN);

    // dma
    dma_hw->ch[0].read_addr = &adc_hw->fifo;
    dma_hw->ch[0].write_addr = &adc_fifo_out;
    dma_hw->ch[0].transfer_count = (1 << DMA_CH0_TRANS_COUNT_MODE_LSB) | 1; 
    dma_hw->ch[0].ctrl_trig = 0;
    uint32_t temp;
    temp = (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | 
           (DREQ_ADC << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | 
           (DMA_CH0_CTRL_TRIG_EN_BITS);
    dma_hw->ch[0].ctrl_trig = temp;

    adc_hw->fcs |= ADC_FCS_DREQ_EN_BITS;
    adc_hw->fcs |= ADC_FCS_EN_BITS;
}