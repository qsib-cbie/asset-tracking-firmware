#ifndef APP_INCLUDE_GPIO_HPP
#define APP_INCLUDE_GPIO_HPP

#include <app_log.hpp>

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include <array>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <tuple>

namespace app_gpio {

struct pin_t {
    std::string_view label;
    int32_t pin;
    int32_t flags;
    const device*  device_binding;

    explicit constexpr pin_t(bool status_okay, std::string_view label, int32_t pin, int32_t flags)
    : label(label), pin(pin), flags(flags), device_binding(nullptr) {
        if(!status_okay) {
            std::string_view message_begin = "Invalid label: ";
            char message[message_begin.size() + label.size()] = {};
            std::memcpy(message, message_begin.begin(), message_begin.size());
            std::memcpy(message + message_begin.size(), label.begin(), label.size());
            throw std::logic_error(message);
        }

    }

    void configure(int32_t extra_flags) {
        LOG_DBG("Configuring %s on %d", label.data(), pin);
        device_binding = device_get_binding(label.data());
        if(gpio_pin_configure(device_binding, pin, extra_flags | flags)) {
            throw std::runtime_error("Failed to configure gpio");
        }
    }

    void set(int value) {
        gpio_pin_set(device_binding, pin, value);
    }
};

template<typename ... T>
struct manager_t {
    std::array<std::tuple<pin_t&, int32_t>, sizeof...(T)> pins;

    explicit constexpr manager_t(T&& ... pin_conf_args)
        : pins(std::array{pin_conf_args...}) {
        for(auto & [pin, flags] : pins) {
            pin.configure(flags);
        }
    }

    void all(bool state) {
        for(const auto& [pin, _] : pins) {
            pin.set(state);
        }
    }
};

using namespace std::literals;

#define PREPARE_GPIO(label) struct label ## _binding_t { \
    static constexpr bool             status_okay = bool{DT_NODE_HAS_STATUS(DT_ALIAS(label), okay)}; \
    static constexpr std::string_view label       = std::string_view{DT_GPIO_LABEL(DT_ALIAS(label), gpios) "\0"}; \
    static constexpr const int32_t   pin          = int32_t{DT_GPIO_PIN(DT_ALIAS(label), gpios)}; \
    static constexpr const int32_t   flags        = int32_t{DT_GPIO_FLAGS(DT_ALIAS(label), gpios)}; \
}; \
app_gpio::pin_t label ## _gpio( \
    label ## _binding_t::status_okay, \
    label ## _binding_t::label, \
    label ## _binding_t::pin, \
    label ## _binding_t::flags);

}


#endif