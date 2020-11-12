#ifndef APP_INCLUDE_APP_TIMER_HPP
#define APP_INCLUDE_APP_TIMER_HPP

#include "zephyr.h"

#include <array>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>

namespace app {

template<size_t SCALE>
struct scale_t { constexpr static size_t value = SCALE; };

template<size_t HZ>
struct hz_t { constexpr static size_t value = HZ; };

template<size_t SEC>
struct sec_delay_t { constexpr static size_t value = SEC * 1'000'000; };

template<size_t MSEC>
struct msec_delay_t { constexpr static size_t value = MSEC * 1'000; };

template<size_t USEC>
struct usec_delay_t { constexpr static size_t value = USEC; };


// Support timer registration and lookup
static constexpr int32_t NUM_TIMERS = 5;
static int32_t TIMER_REGISTRATION_INDEX = 0;
static std::array<std::tuple<k_timer*, std::function<void()>>,NUM_TIMERS> handlers;

void timer_handler(k_timer* timer) {
    for(const auto & [registered_timer, handler] : handlers) {
        if(registered_timer == timer) {
            handler();
        }
    }
}

// Accept work as a labmda, providing static callbacks and holding references in RAII fashion
template<size_t I = 0>
struct lambda_work_t {
private:
	static std::function<void()> s_work;

public:
    struct inner_t {
        bool m_destruct;
        constexpr inner_t(std::function<void()>&& f, bool destruct) : m_destruct(destruct) {
            LOG_DBG("Called inner %d ctr", (int) I);
            s_work = std::move(f);
        }

        constexpr inner_t(inner_t&& rhs) : m_destruct(true) {
            rhs.m_destruct = false;
        }

        inner_t(const inner_t&) = delete;
        inner_t() = delete;

        ~inner_t() {
            LOG_DBG("Called inner %d dtr", (int) I);
            if(m_destruct) {
                s_work = [](){ LOG_DBG("Called inner %d after destruction", (int) I); };
            }
        }
    };
    static_assert(!std::is_copy_constructible<inner_t>::value, "not copyable");

	static void submit() {
		k_work_init(&work, lambda_work_handler);
        // Submit to system or app work queue
        if (work_q == nullptr) {
            LOG_DBG("Submitting work to system workqueue");
            k_work_submit(&work);
        } else {
            LOG_DBG("Submitting work to special workqueue: %p", work_q.get());
            k_work_submit_to_queue(work_q.get(), &work);
        }
	}

	static void lambda_work_handler(k_work* item) {
		s_work();
	}

    static k_work work;
    static std::shared_ptr<k_work_q> work_q;

private:
    inner_t m_inner;

public:
    constexpr lambda_work_t(inner_t&& inner) : m_inner(std::move(inner)) {}
    constexpr lambda_work_t(lambda_work_t&& rhs) : m_inner(std::move(rhs.m_inner)) {}
    lambda_work_t(const lambda_work_t&) = delete;
};

// Registers a timer and and calls handlers in RAII fashion
template<typename THZ, typename TSCALER = scale_t<1>>
struct timer_t {
private:
    k_timer m_timer;
    bool m_stopped;

public:
    template<typename TLAMBDA>
    timer_t(TLAMBDA&& work)
        : m_timer(),
          m_stopped(true) {

        if (TIMER_REGISTRATION_INDEX == NUM_TIMERS) {
            throw new std::logic_error("Too many registered timers");
        }

        LOG_INF("Registering timer %d", (int) TIMER_REGISTRATION_INDEX);
        const auto lambda_work = std::make_shared<TLAMBDA>(std::move(work));
        handlers[TIMER_REGISTRATION_INDEX++] = std::make_tuple<k_timer*, std::function<void()>>(&m_timer, [lambda_work]() { TLAMBDA::submit(); });

        k_timer_init(&m_timer, timer_handler, NULL);
        k_timer_start(&m_timer, K_USEC(1'000'000 / THZ::value), K_USEC(TSCALER::value * 1'000'000 / THZ::value));
        m_stopped = false;
    }

    ~timer_t() {
        stop();
    }

    void stop() {
        if(!m_stopped) {
            k_timer_stop(&m_timer);
            m_stopped = true;
        }
    }
};

// Registers a one shot timer and and calls handlers in RAII fashion
template<typename TDELAY>
struct one_shot_timer_t {
private:
    k_timer m_timer;
    bool m_stopped;

public:
    template<typename TLAMBDA>
    one_shot_timer_t(TLAMBDA&& work)
        : m_timer(),
          m_stopped(true) {

        if (TIMER_REGISTRATION_INDEX == NUM_TIMERS) {
            throw new std::logic_error("Too many registered timers");
        }
        LOG_INF("Registering timer %d", (int) TIMER_REGISTRATION_INDEX);
        const auto lambda_work = std::make_shared<TLAMBDA>(std::move(work));
        handlers[TIMER_REGISTRATION_INDEX++] = std::make_tuple<k_timer*, std::function<void()>>( &m_timer, [lambda_work]() { TLAMBDA::submit(); });

        k_timer_init(&m_timer, timer_handler, NULL);
        k_timer_start(&m_timer, K_USEC(TDELAY::value), K_SECONDS(0));
        m_stopped = false;
    }

    ~one_shot_timer_t() {
        stop();
    }

    void stop() {
        if(!m_stopped) {
            k_timer_stop(&m_timer);
            m_stopped = true;
        }
    }
};


}

#endif