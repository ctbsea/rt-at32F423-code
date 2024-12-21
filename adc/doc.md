## rt-thread adc dma 

adc不间断自动采集， 有3个通道被监听

### 关键参数说明
```
// 3个通道， 监听3批次数据
rt_uint16_t values[9];

// 不间断自动采集 triger触发只需要一次即可
adc_base_struct.repeat_mode = TRUE;
adc_ordinary_software_trigger_enable(adc_x, TRUE);

// adc_dma_request_repeat_enable() 函数的作用是控制 ADC 转换完成后，是否自动重复触发 DMA 请求。
// TRUE（启用重复请求模式）：
// 当 ADC 完成一次转换时，自动生成 DMA 请求，并可以在循环模式下持续进行数据采样。
// FALSE（禁用重复请求模式）：
// 当 ADC 完成一次转换时，只生成一次 DMA 请求。要继续采样，需要通过软件或其他触发方式再次启动 ADC
// false的时候 DMA Full 事件不会触发的 。
adc_dma_request_repeat_enable(adc_x, TRUE);

// 开启循环模式, values 会被一直覆盖写入,  没写满一次会触发一次中断
dma_init_struct.loop_mode_enable = TRUE;
```

### 运行
```

rt_uint16_t values[9];

rt_err_t callBack(rt_device_t dev, rt_size_t size)
{
    rt_kprintf("%s \n", "==============================");
    rt_kprintf("1: %d %d %d  \n", values[0], values[1], values[2]);
    rt_kprintf("2: %d %d %d  \n", values[3], values[4], values[5]);
    rt_kprintf("3: %d %d %d  \n", values[6], values[7], values[8]);
    return RT_EOK;
}

int madc(void)
{
    gpio_config();

    rt_adc_device_t adc_dev;
    rt_err_t ret = RT_EOK;

    /* 查找设备 */
    adc_dev = (rt_adc_device_t) rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
        return RT_ERROR;
    }

    adc_dma_config(adc_dev, (rt_uint32_t) values, 9);
    
    rt_device_set_rx_indicate(adc_dev, callBack);
    at32_adc_dma_enabled(adc_dev);
    at32_start_trigger(adc_dev);
    return RT_EOK;
}
```

### 结果

```
1: 2058 2043 2061
2: 2054 2050 2038
3: 2047 2057 2048

1: 2058 2046 2063
2: 2057 2045 2064
3: 2042 2058 2048
1: 2043 2044 2056
2: 2038 2062 2044
3: 2060 2043 2051

1: 2059 2041 2060
2: 2051 2045 2062
3: 2051 2046 2061
```
