#pragma once

#include <chrono>
#include <iostream>

#define HEX 0
#define DEC 1

#include <inttypes.h>
typedef uint8_t byte;

void begin();
void loop();

int main()
{
    begin();

    while (true)
    {
        loop();
    }
}

// avoid strncpy security warning
#pragma warning(disable:4996)

#define __attribute__(A) /* do nothing */

#include <midi_Defs.h>

float analogRead(int pin)
{
    return 0.0f;
}

void randomSeed(float)
{
    srand(static_cast<unsigned int>(time(0)));
}

unsigned long millis()
{
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return (unsigned long)now;
}

int random(int min, int max)
{
    return RAND_MAX % std::rand() % (max - min) + min;
}

template <class T> const T& min(const T& a, const T& b) {
    return !(b < a) ? a : b;     // or: return !comp(b,a)?a:b; for version (2)
}

#define F(x) x
