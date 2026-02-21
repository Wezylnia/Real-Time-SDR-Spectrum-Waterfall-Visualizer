# Real-Time SDR Spectrum & Waterfall Visualizer

This project is a real-time spectrum and waterfall visualizer for RTL-SDR devices. It captures I/Q data from the SDR, computes the power spectral density (PSD) using an FFT
and displays the results as a spectrum plot and a waterfall diagram.

The user interface is built with SDL2 and provides a control panel to adjust parameters like frequency, sample rate, and gain.

## Features

*   **Real-time Spectrum Display:** Visualizes the frequency spectrum of the received signal.
*   **Waterfall Display:** Shows the history of the spectrum over time, allowing for the identification of transient signals.
*   **Interactive Control Panel:** Allows for on-the-fly adjustments of SDR parameters.
*   **IQ Data Recording:** Can record the raw I/Q data for later analysis.

## Modules

The project is divided into the following modules:

*   `sdr`: Handles communication with the RTL-SDR device.
*   `fft`: Performs the Fast Fourier Transform (FFT) and power spectral density (PSD) calculation.
*   `render`: Manages the SDL2-based rendering of the spectrum, waterfall, and UI elements.
*   `panel`: Implements the control panel layout and event handling.
*   `widgets`: Provides UI elements like sliders, buttons, and text inputs.
*   `recorder`: Manages background I/Q data recording.
*   `main`: Integrates all modules and runs the main application loop.

## Building

To build the project, you will need an MSYS2 MinGW 64-bit environment. Once the environment is set up, you can build the project by running `make` in the project directory.

## Usage

After building, run the executable. The application will start and display the spectrum and waterfall display.

### Keyboard Shortcuts

*   **Left/Right Arrows:** Adjust frequency by ±1 MHz.
*   **Up/Down Arrows:** Adjust frequency by ±100 kHz.
*   **ESC:** Exit the application.
