#ifndef APP_INCLUDE_APP_MEASURE_HPP
#define APP_INCLUDE_APP_MEASURE_HPP

#include <zephyr.h>
#include <device.h>
#include <drivers/adc.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_saadc.h>

#include <array>

namespace app {
    struct adc_t {
        static constexpr size_t NUM_CHANNELS = 1;
		static constexpr uint16_t ADC_ACQUISITION_TIME = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10);

        adc_channel_cfg vdd_channel_cfg = {
            .gain             = ADC_GAIN_1_6,
            .reference        = ADC_REF_INTERNAL,
            .acquisition_time = ADC_ACQUISITION_TIME,
            .channel_id       = 0,
            .differential     = 0,
            .input_positive   = NRF_SAADC_INPUT_VDD
        };
    };
}

#endif
