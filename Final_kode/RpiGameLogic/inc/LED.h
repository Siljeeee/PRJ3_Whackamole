#ifndef LED_H
#define LED_H

#include <gpiod.h>
#include <vector>
#include <string>
#include <mutex>

class LED
{
public:
    LED(const std::vector<int>& pins, const std::string& chipPath = "/dev/gpiochip15");
    ~LED();

    void activate(int n);
    void turnOffAll();
    void blinkAll(int times = 3);

private:
    void writeLine(int n, bool on);

    std::vector<int> pins_;
    std::string chipPath_;
    std::vector<unsigned int> offsets_;
    struct gpiod_chip* chip_ = nullptr;
    struct gpiod_line_request* request_ = nullptr;
    std::mutex mtx_;
};

#endif
