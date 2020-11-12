#ifndef APP_INCLUDE_APP_SAADC_HPP
#define APP_INCLUDE_APP_SAADC_HPP

#include <app_battery.hpp>
#include <app_log.hpp>

#include <app/measure.hpp>

#include <zephyr.h>
#include <device.h>
#include <drivers/adc.h>
#include <drivers/sensor.h>

#include <hal/nrf_gpio.h>
#include <hal/nrf_saadc.h>

#include <array>
#include <cstring>
#include <stdexcept>

namespace app_saadc {

	template<size_t NUM_CHANNELS = app::adc_t::NUM_CHANNELS>
	struct manager_t {
		int16_t sample_buffer[NUM_CHANNELS];

		constexpr manager_t() : sample_buffer() {
			memset(sample_buffer, 0, sizeof(sample_buffer));

			// Calibration on construction
			const app::adc_t adc_confs;
			measure(std::array{ &adc_confs.vdd_channel_cfg, }, true);
		}

		// One-shot measurements
		template<size_t SAMPLES>
		std::array<int32_t, SAMPLES> measure(std::array<const adc_channel_cfg*, SAMPLES>&& configs, bool calibrate) {
			const device* adc_device = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_saadc)));

			uint8_t channels = 0;
			int ret;
			for(int i = 0; i < (int) SAMPLES; i++) {
				LOG_DBG("Channel: %d on PinP: %d PinN: %d", (int32_t) configs[i]->channel_id, (int32_t) configs[i]->input_positive, (int32_t) configs[i]->input_negative);
				channels |= BIT(configs[i]->channel_id);
				ret = adc_channel_setup(adc_device, configs[i]);
				if(ret) {
					LOG_ERR("Failed to register channel config for %d", i);
					throw std::runtime_error("Failed to register channel");
				}
			}

			// Initiate the saadc sequence
			memset(sample_buffer, 0, sizeof(sample_buffer));
			const adc_sequence sequence = {
				.channels     = channels,
				.buffer       = sample_buffer,
				.buffer_size  = sizeof(sample_buffer),
				.resolution   = configs.size() == 1 ? 14 : 12,
				.oversampling = configs.size() == 1 ? 4 : 0,
				.calibrate    = calibrate
			};
			ret = adc_read(adc_device, &sequence);
			if(ret) {
				LOG_ERR("Failed to do an adc_read: %d", ret);
				throw std::runtime_error("Failed to do adc_read");
			}

			// Convert the samples to meaningful scale (mV)
			std::array<int32_t, SAMPLES> measurements;
			for(int i = 0; i < (int) SAMPLES; i++) {
				measurements[i] = static_cast<int32_t>(sample_buffer[i]);
				ret = adc_raw_to_millivolts(adc_ref_internal(adc_device),
									configs[i]->gain,
									sequence.resolution - configs[i]->differential,
									&measurements[i]);
				LOG_DBG("Channel: %" PRId32" Before: %" PRId32 " After: %" PRId32, configs[i]->channel_id, (int) sample_buffer[i], (int) measurements[i]);
				if (ret) {
					LOG_ERR("Failed to convert adc raw to millivolts: ret %d", ret);
					throw std::runtime_error("Failed to convert adc");
				}
			}

			return measurements;
		}
	};
}

#endif
