#include "ForceSensor.h"

#include <stdexcept>
#include <thread>
#include <chrono>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>


ForceSensor::ForceSensor(
    const std::vector<int>& channels,
    int threshold,
    int adsAddress,
    const std::string& device
)
    : channels_(channels), threshold_(threshold),
      adsAddress_(adsAddress), device_(device)
{
    i2cFile_ = open(device_.c_str(), O_RDWR);
    if (i2cFile_ < 0)
        throw std::runtime_error("Could not open I2C device: " + device_);

    if (ioctl(i2cFile_, I2C_SLAVE, adsAddress_) < 0)
        throw std::runtime_error("Could not connect to ADS1015 at I2C address");
}

ForceSensor::~ForceSensor()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (i2cFile_ >= 0)
    {
        close(i2cFile_);
        i2cFile_ = -1;
    }
}

void ForceSensor::writeRegister(unsigned char reg, unsigned short val)
{
    unsigned char buf[3];
    buf[0] = reg;
    buf[1] = static_cast<unsigned char>((val >> 8) & 0xFF);
    buf[2] = static_cast<unsigned char>(val & 0xFF);

    if (write(i2cFile_, buf, 3) != 3)
        throw std::runtime_error("Could not write to ADS1015 register");
}

unsigned short ForceSensor::readRegister(unsigned char reg)
{
    if (write(i2cFile_, &reg, 1) != 1)
        throw std::runtime_error("Could not select ADS1015 register");

    unsigned char buf[2];
    if (read(i2cFile_, buf, 2) != 2)
        throw std::runtime_error("Could not read ADS1015 register");

    return static_cast<unsigned short>((buf[0] << 8) | buf[1]);
}

int ForceSensor::readChannel(int ch)
{
    if (ch < 0 || ch > 3)
        throw std::runtime_error("ADS1015 channel must be 0-3");

    std::lock_guard<std::mutex> lock(mtx_);
   
    static const uint16_t muxTable[] = { 0x4000, 0x5000, 0x6000, 0x7000 };

    uint16_t config =
        0x8000        |   // start single conversion
        muxTable[ch]  |   // select channel
        0x0200        |   // PGA ±4.096V
        0x0100        |   // single-shot mode
        0x0080        |   // 1600 SPS
        0x0003;           // comparator disabled

    unsigned short raw = readRegister(0x00);
    int val = static_cast<int>(raw) >> 4;

    writeRegister(0x01, config);

// Vent til ADS1015 er færdig med conversion
for (int i = 0; i < 20; i++)
{
    unsigned short cfg = readRegister(0x01); //læser at målingen er færdig via status bit i config

    if (cfg & 0x8000) // OS-bit = 1 betyder færdig
        break;

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

    if (val & 0x0800) val |= 0xF000;

    return val;
}


// undersøg om specifik sensor 'i' er aktiv
bool ForceSensor::isActive(int sensor)
{
    // error handling 
    if (sensor < 0 || sensor >= static_cast<int>(channels_.size())) return false;

    return readChannel(channels_[sensor]) > threshold_;
}

bool ForceSensor::anyActive()
{
    for (int i = 0; i < static_cast<int>(channels_.size()); i++)
    {
        if (isActive(i)){
             return true;
        }
    }
    return false;
}

int ForceSensor::scanWithinTime(int timeoutMs, const std::atomic<bool>& running)
{
    using namespace std::chrono;
    auto start = steady_clock::now();

    while (running.load())
    {
        int elapsed = static_cast<int>(
            duration_cast<milliseconds>(steady_clock::now() - start).count()
        );
        if (elapsed >= timeoutMs) return -1;

        int bestSensor = -1;
        int bestValue  = threshold_;

        for (int i = 0; i < static_cast<int>(channels_.size()); i++)
        {
            try
            {
                int val = readChannel(channels_[i]);
                if (val > bestValue)
                {
                    bestValue  = val;
                    bestSensor = i;
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Sensor " << i << " read error: " << e.what() << "\n";
            }
        }

        if (bestSensor >= 0) return bestSensor;

        std::this_thread::sleep_for(milliseconds(100));
    }

    return -1;
}
