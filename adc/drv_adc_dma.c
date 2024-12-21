#include "drv_adc.h"

#if defined(BSP_USING_ADC1_DMA)

//#define DRV_DEBUG
#define LOG_TAG             "drv.adc"
#include <drv_log.h>

struct at32_adc
{
    struct rt_adc_device at32_adc_device;
    adc_type *adc_x;
    char *name;
};

enum
{
    ADC1_INDEX,
};

static struct at32_adc at32_adc_obj[] = { ADC1_CONFIG };

rt_err_t gpio_config(void)
{
    gpio_init_type gpio_initstructure;
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    gpio_default_para_init(&gpio_initstructure);
    gpio_initstructure.gpio_mode = GPIO_MODE_ANALOG;
    gpio_initstructure.gpio_pins = GPIO_PINS_4 | GPIO_PINS_5 | GPIO_PINS_6;
    gpio_init(GPIOA, &gpio_initstructure);
}

void adc_dma_config(struct rt_adc_device *device, rt_uint32_t memory_base_addr, rt_uint16_t buffer_size)
{
    adc_type *adc_x;

    RT_ASSERT(device != RT_NULL);
    adc_x = device->parent.user_data;

    dma_init_type dma_init_struct;

    crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);
    nvic_irq_enable(DMA1_Channel1_IRQn, 0, 0);

    dma_reset(DMA1_CHANNEL1);

    dma_default_para_init(&dma_init_struct);
    dma_init_struct.buffer_size = buffer_size;
    dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_base_addr = memory_base_addr;
    dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_HALFWORD;
    dma_init_struct.memory_inc_enable = TRUE;
    dma_init_struct.peripheral_base_addr = (rt_uint32_t) &(adc_x->odt);
    dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
    dma_init_struct.peripheral_inc_enable = FALSE;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init_struct.loop_mode_enable = TRUE;
    dma_init(DMA1_CHANNEL1, &dma_init_struct);

    dmamux_enable(DMA1, TRUE);
    dmamux_init(DMA1MUX_CHANNEL1, DMAMUX_DMAREQ_ID_ADC1);

    /* enable dma transfer complete interrupt */
    dma_interrupt_enable(DMA1_CHANNEL1, DMA_FDT_INT, TRUE);
    dma_channel_enable(DMA1_CHANNEL1, TRUE);
}

rt_err_t at32_adc_dma_enabled(struct rt_adc_device *device)
{
    adc_type *adc_x;
    RT_ASSERT(device != RT_NULL);
    adc_x = device->parent.user_data;

    adc_common_config_type adc_common_struct;
    adc_base_config_type adc_base_struct;

    crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK, TRUE);
    nvic_irq_enable(ADC1_IRQn, 0, 0);
    crm_adc_clock_select(CRM_ADC_CLOCK_SOURCE_HCLK);

    adc_common_default_para_init(&adc_common_struct);
    adc_common_struct.div = ADC_HCLK_DIV_4;
    adc_common_struct.tempervintrv_state = FALSE;
    adc_common_config(&adc_common_struct);

    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.sequence_mode = TRUE;
    adc_base_struct.repeat_mode = TRUE;
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 3;
    adc_base_config(adc_x, &adc_base_struct);

    adc_resolution_set(adc_x, ADC_RESOLUTION_12B);

    /* config ordinary channel */
    adc_ordinary_channel_set(adc_x, ADC_CHANNEL_4, 1, ADC_SAMPLETIME_92_5);
    adc_ordinary_channel_set(adc_x, ADC_CHANNEL_5, 2, ADC_SAMPLETIME_92_5);
    adc_ordinary_channel_set(adc_x, ADC_CHANNEL_6, 3, ADC_SAMPLETIME_92_5);

    /* config ordinary trigger source and trigger edge */
    adc_ordinary_conversion_trigger_set(adc_x, ADC_ORDINARY_TRIG_SOFTWARE, ADC_ORDINARY_TRIG_EDGE_NONE);

    adc_dma_mode_enable(adc_x, TRUE);
    adc_dma_request_repeat_enable(adc_x, TRUE);

    adc_interrupt_enable(adc_x, ADC_OCCO_INT, TRUE);

    /* adc enable */
    adc_enable(adc_x, TRUE);
    while (adc_flag_get(adc_x, ADC_RDY_FLAG) == RESET)
        ;
    /* adc calibration */
    adc_calibration_init(adc_x);
    while (adc_calibration_init_status_get(adc_x))
        ;
    adc_calibration_start(adc_x);
    while (adc_calibration_status_get(adc_x))
        ;
    return RT_EOK;
}

rt_err_t at32_start_trigger(struct rt_adc_device *device)
{
    adc_type *adc_x;
    RT_ASSERT(device != RT_NULL);
    adc_x = device->parent.user_data;

    adc_ordinary_software_trigger_enable(adc_x, TRUE);
    return RT_EOK;
}

void adc_dma_rx_isr(struct rt_adc_device *device)
{
    rt_kprintf("flag %d", dma_flag_get(DMA1_FDT1_FLAG));
    while (dma_flag_get(DMA1_FDT1_FLAG) != RESET)
    {
        dma_flag_clear(DMA1_FDT1_FLAG);
        if (device->parent.rx_indicate != RT_NULL)
        {
            device->parent.rx_indicate(&device->parent, 9);
        }
    }
}

void DMA1_Channel1_IRQHandler(void)
{
    rt_interrupt_enter();
    adc_dma_rx_isr(&at32_adc_obj[ADC1_INDEX].at32_adc_device);
    rt_interrupt_leave();
}

static rt_err_t at32_adc_enabled(struct rt_adc_device *device, rt_int8_t channel, rt_bool_t enabled)
{
    return -RT_ENOSYS;
}
static rt_err_t at32_get_adc_value(struct rt_adc_device *device, rt_int8_t channel, rt_uint32_t *value)
{
    return -RT_ENOSYS;
}

static const struct rt_adc_ops at_adc_ops = { .enabled = at32_adc_enabled, .convert = at32_get_adc_value, };

static int rt_hw_adc_init(void)
{
    int result = RT_EOK;

    if (rt_hw_adc_register(&at32_adc_obj[0].at32_adc_device, at32_adc_obj[0].name, &at_adc_ops,
            at32_adc_obj[0].adc_x) == RT_EOK)
    {
        LOG_D("%s register success", at32_adc_obj[0].name);
    }
    else
    {
        LOG_E("%s register failed", at32_adc_obj[0].name);
        result = -RT_ERROR;
    }
    return result;
}
INIT_BOARD_EXPORT(rt_hw_adc_init);

#endif /* BSP_USING_ADC */
