#ifndef FORCESENSOR_H
#define FORCESENSOR_H

#include <vector>
#include <string>
#include <mutex>
#include <atomic>

class ForceSensor
{
public:
    ForceSensor(
        const std::vector<int>& channels,
        int threshold,
        int adsAddress = 0x48,
        const std::string& device = "/dev/i2c-1"
    );
    ~ForceSensor();

    // Scans all sensors; returns the index of the sensor with the highest
    // reading above threshold, or -1 on timeout. Uses highest-reading wins
    // so a noisy early channel cannot mask a real hit on a later channel.
    int scanWithinTime(int timeoutMs, const std::atomic<bool>& running);

    bool isActive(int sensor);
    bool anyActive();
 
    int readChannel(int ch);
private:
    void writeRegister(unsigned char reg, unsigned short val);
    unsigned short readRegister(unsigned char reg);

    std::vector<int> channels_;
    int threshold_;
    int adsAddress_;
    std::string device_;
    int i2cFile_ = -1;
    std::mutex mtx_;
};

#endif
