// This file is part of the stenosaurus project.
//
// Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
//
// This library is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this library.  If not, see <http://www.gnu.org/licenses/>.
//
// This file is the main entry point for the stenosaurus firmware.

#include "../common/user_button.h"
#include "clock.h"
#include "debug.h"
#include "keyboard.h"
#include "protocol.h"
#include "sdio.h"
#include "stroke.h"
#include "txbolt.h"
#include "usb.h"
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/crc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencmsis/core_cm3.h>

#include "../common/leds.h"

void NkroButtonTest(void) {
    setup_user_button();
    setup_leds();

    bool pressed = false;

    while (true) {
        if (is_user_button_pressed()) {
            if (!pressed) {
                led_toggle(0);
                usb_keyboard_key_down(KEY_Q);
                usb_keyboard_key_down(KEY_W);
                usb_keyboard_key_down(KEY_E);
                usb_keyboard_key_down(KEY_R);
                usb_keyboard_key_down(KEY_U);
                usb_keyboard_key_down(KEY_I);
                usb_keyboard_key_down(KEY_O);
                usb_keyboard_key_down(KEY_P);
                pressed = true;                
            }
        } else {
            if (pressed) {
                usb_keyboard_key_up(KEY_Q);
                usb_keyboard_key_up(KEY_W);
                usb_keyboard_key_up(KEY_E);
                usb_keyboard_key_up(KEY_R);
                usb_keyboard_key_up(KEY_U);
                usb_keyboard_key_up(KEY_I);
                usb_keyboard_key_up(KEY_O);
                usb_keyboard_key_up(KEY_P);
                led_toggle(0);
                pressed = false;                
            }
        }
        usb_send_keys_if_changed();
    }
}

void SdcardTest(void) {
    sdio_init();

    bool card_initialized = false;

    while (true) {
        if (sdio_card_present() && !card_initialized) {
            print("Card detected.\r\n");

            if (!sdio_card_init()) {
                print("Card not initialized.\r\n\r\n");
                continue;
            }

            print("Initialized card.\r\n\r\n");

            card_initialized = true;
        }
        if (!sdio_card_present() && card_initialized) {
            print("Card removed.\r\n\r\n");
            card_initialized = false;
        }
    }
}

void TxboltTest(void) {
    setup_user_button();
    setup_leds();

    bool pressed = false;

    while (true) {
        if (is_user_button_pressed()) {
            if (!pressed) {
                packet txbolt_packet;
                uint32_t stroke = string_to_stroke("PHRO*FR");
                make_packet(stroke, &txbolt_packet);
                usb_send_serial_data(&txbolt_packet.byte[0], 
                                     txbolt_packet.length);
                pressed = true;
            }
        } else {
            if (pressed) {
                pressed = false;
            }
        }
    }
}

void TypingTest(void) {
    setup_user_button();
    setup_leds();

    bool pressed = false;

    while (true) {
        keyboard_poll();

        if (is_user_button_pressed()) {
            if (!pressed) {
                led_toggle(0);
                keyboard_type_string("Hello. ");

                pressed = true;
            }
        } else {
            if (pressed) {
                pressed = false;
            }
        }
    }
}

int main(void) {
    clock_init();
    usb_init(packet_handler);

    // Use of the LEDs and SDIO are mutually exclusive.

    const uint32_t JOY_PORT = GPIOA;
    const uint32_t JOY_LEFT = GPIO0;
    const uint32_t JOY_UP = GPIO1;
    const uint32_t JOY_DOWN = GPIO2;
    const uint32_t JOY_RIGHT = GPIO3;
    const uint32_t JOY_CENTER = GPIO4;
    const uint32_t JOY_ALL = (
        JOY_LEFT | JOY_UP | JOY_DOWN | JOY_RIGHT | JOY_CENTER);

    // Configure the joystick.
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    gpio_set_mode(
        JOY_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, JOY_ALL);
    gpio_set(JOY_PORT, JOY_ALL);

    // For some reason the initial state of the joystick is wrong.
    wait(1000);

    while (true) {
        uint32_t joy_state = (~GPIOA_IDR) & JOY_ALL;

        if (joy_state & JOY_UP) {
            NkroButtonTest();
        }
        if (joy_state & JOY_LEFT) {
            SdcardTest();
        }
        if (joy_state & JOY_RIGHT) {
            TxboltTest();
        }
        if (joy_state & JOY_DOWN) {
            TypingTest();
        }
        if (joy_state & JOY_CENTER) {
        }
    }
}
