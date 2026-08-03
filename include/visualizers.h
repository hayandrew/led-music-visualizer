#ifndef VISUALIZERS_H
#define VISUALIZERS_H

#include <Arduino.h>
#include <FastLED.h>
#include "project_config.h"

// Global LED buffer and serpentine indexing helper exposed to visualizers
extern CRGB leds[NUM_LEDS];
uint16_t getLEDIndex(uint8_t x, uint8_t y);

void drawDiagnosticHeart();
void drawSpectrumLinear();
void drawSpectrumSymmetric();
void drawVUMeter();
void drawBassPulse();
void drawSoundRipples();
void drawNoise();
void drawRainbowWave();
void drawFirePortal();
void drawDigitalRain();
void drawPulsingTunnel();
void drawMarioRun();
void drawLavaLamp();
void drawAudioPlasma();
void drawAudioParticles();

#endif // VISUALIZERS_H
