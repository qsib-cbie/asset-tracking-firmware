#ifndef APP_INCLUDE_APP_WORK_HPP
#define APP_INCLUDE_APP_WORK_HPP

#include <app/timer.hpp>

using base_work_t = app::lambda_work_t<0>;
template<> std::function<void()> base_work_t::s_work = [](){ LOG_DBG("base_work_t::s_work init"); };
template<> k_work base_work_t::work = k_work();
template<> std::shared_ptr<k_work_q> base_work_t::work_q = nullptr;

using wake_work_t = app::lambda_work_t<1>;
template<> std::function<void()> wake_work_t::s_work = [](){ LOG_DBG("wake_work_t::s_work init"); };
template<> k_work wake_work_t::work = k_work();
template<> std::shared_ptr<k_work_q> wake_work_t::work_q = nullptr;

#endif