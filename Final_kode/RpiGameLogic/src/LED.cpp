#include "LED.h"

#include <stdexcept>
#include <thread>
#include <chrono>

LED::LED(const std::vector<int>& pins, const std::string& chipPath)
    : pins_(pins), chipPath_(chipPath)
{
    chip_ = gpiod_chip_open(chipPath_.c_str());
    if (!chip_)
        throw std::runtime_error("Could not open GPIO chip: " + chipPath_);

    for (int p : pins_)
        offsets_.push_back(static_cast<unsigned int>(p));

    auto* settings = gpiod_line_settings_new();
    auto* config   = gpiod_line_config_new();
    auto* reqcfg   = gpiod_request_config_new();

    if (!settings || !config || !reqcfg)
        throw std::runtime_error("Could not allocate LED GPIO settings");

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    if (gpiod_line_config_add_line_settings(
            config, offsets_.data(), offsets_.size(), settings) < 0)
        throw std::runtime_error("Could not configure LED GPIO lines");

    gpiod_request_config_set_consumer(reqcfg, "whack_led");

    request_ = gpiod_chip_request_lines(chip_, reqcfg, config);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(config);
    gpiod_request_config_free(reqcfg);

    if (!request_)
        throw std::runtime_error("Could not request LED GPIO lines");
}

LED::~LED()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (request_)
    {
        for (auto off : offsets_)
            gpiod_line_request_set_value(request_, off, GPIOD_LINE_VALUE_INACTIVE);
        gpiod_line_request_release(request_);
    }
    if (chip_)
        gpiod_chip_close(chip_);
}

void LED::writeLine(int n, bool on)
{
    if (n < 0 || n >= static_cast<int>(offsets_.size())) return;
    auto val = on ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
    gpiod_line_request_set_value(request_, offsets_[n], val);
}

void LED::activate(int n)
{
    std::lock_guard<std::mutex> lock(mtx_);
    for (int i = 0; i < static_cast<int>(offsets_.size()); i++)
        writeLine(i, i == n);
}

void LED::turnOffAll()
{
    std::lock_guard<std::mutex> lock(mtx_);
    for (int i = 0; i < static_cast<int>(offsets_.size()); i++)
        writeLine(i, false);
}

void LED::blinkAll(int times)
{
    using namespace std::chrono_literals;
    for (int i = 0; i < times; i++)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            for (int j = 0; j < static_cast<int>(offsets_.size()); j++)
                writeLine(j, true);
        }
        std::this_thread::sleep_for(300ms);
        {
            std::lock_guard<std::mutex> lock(mtx_);
            for (int j = 0; j < static_cast<int>(offsets_.size()); j++)
                writeLine(j, false);
        }
        std::this_thread::sleep_for(300ms);
    }
}
