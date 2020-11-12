#include <app_log.hpp>
#include <app_exception.hpp>
#include <app_gpio.hpp>
#include <app_ble.hpp>
#include <app_saadc.hpp>
#include <app_system_off.hpp>
#include <app_lfs.hpp>

#include <app/version.hpp>
#include <app/work.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <numeric>
#include <tuple>

static K_THREAD_STACK_DEFINE(wake_work_stack, 2048);


enum class app_state_e {
	ERROR = 0,
	OK = 1,
	DONE = 2
};

template<typename ... T>
static void notify_error(const char* msg, const T ... msg_args) {
	std::string message(128, '\0');
	size_t message_length = snprintf(message.data(), message.size(), msg, msg_args...);
	message.resize(message_length + 1);
	ass_error_write(std::string_view{message});
}

void main() {
	LOG_INF("Version %s: Beginning main() ...", VERSION);

	// // Enable wake from deep sleep over gpio
	// nrf_gpio_cfg_input(DT_GPIO_PIN(DT_NODELABEL(custombutton), gpios), NRF_GPIO_PIN_PULLUP);
	// nrf_gpio_cfg_sense_set(DT_GPIO_PIN(DT_NODELABEL(custombutton), gpios), NRF_GPIO_PIN_SENSE_LOW);

	// Prepare the rest of the hardware managers
	app_ble::manager_t ble_manager;
	app_lfs::manager_t lfs_manager;
	app_saadc::manager_t saadc_manager;

	k_sleep(K_SECONDS(2));

	lfs_manager.read("%s/value", (char*) ass_value, sizeof(ass_value));
	lfs_manager.read("%s/data", (char*) ass_data, sizeof(ass_data));

	// Initialize the rest of the shared app state
	k_work_q _wake_work_q;
	std::shared_ptr<k_work_q> wake_work_q = std::shared_ptr<k_work_q>(&_wake_work_q, [](k_work_q*){});
	k_work_q_start(wake_work_q.get(), wake_work_stack, K_THREAD_STACK_SIZEOF(wake_work_stack), 1);
	app_state_e state = app_state_e::OK;

	// Proceed with measurements
	constexpr size_t ADV_WAKE_PERIOD = 20;
	constexpr size_t ADV_WAKE_DUTY_CYCLE = 80;
	constexpr app::adc_t adc_conf;
	{
		const auto do_wake = [&]() {
			if(ass_updated) {
				char value[sizeof(ass_value) + 1];
				memcpy(value, ass_value, sizeof(ass_value));
				value[sizeof(ass_value)] = 0;
				lfs_manager.write_value(std::string_view(value, strlen(value)));

				char data[sizeof(ass_data) + 1];
				memcpy(data, ass_data, sizeof(ass_data));
				data[sizeof(ass_data)] = 0;
				lfs_manager.write_data(std::string_view(data, strlen(data)));
			}

			const auto samples = saadc_manager.measure(std::array{&adc_conf.vdd_channel_cfg}, true);
			bt_bas_set_battery_level(battery_level_pct(samples[0]));

			LOG_INF("Start advertising");
			ble_manager.start();
			k_sleep(K_SECONDS(ADV_WAKE_PERIOD * ADV_WAKE_DUTY_CYCLE / 100));
			LOG_INF("Stop advertising");
			ble_manager.stop();
		};
		wake_work_t wake_work(wake_work_t::inner_t(std::move(do_wake), true));
		wake_work_t::work_q = wake_work_q;

		// Run app lifecycle
		const auto wake_timer = app::timer_t<app::hz_t<1>, app::scale_t<ADV_WAKE_PERIOD>>(std::move(wake_work));

		// Allow lifecycle to happen
		while (state == app_state_e::OK) { k_sleep(K_MSEC(100)); }

		LOG_DBG("Leaving lifecycle scope");

		// Fallthrough scope to cleanup measurement context
	}

	LOG_INF("Destroyed destroyed scope");

	// Enter deep sleep
	power_off();

	// Prevent fall through
	NVIC_SystemReset();
}
