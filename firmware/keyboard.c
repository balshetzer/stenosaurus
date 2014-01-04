// This file is part of the stenosaurus project.
//
// Copyright (C) 2014 Hesky Fisher <hesky.fisher@gmail.com>
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
// This file implements keyboard related functionality for the Stenosaurus. See
// the header file for interface documentation to this code.

#include "keyboard.h"

#include "clock.h"
#include "usb.h"
#include <stdint.h>

const uint32_t KEY_TIMEOUT_MS = 50;

#define FIFO_SIZE 1024
static struct {
	uint32_t front;
	uint32_t count;
	uint8_t data[FIFO_SIZE];
} key_fifo;

static uint32_t deadline_for_key;

static void keyboard_fifo_push(uint8_t b) {
	uint32_t write_index = (key_fifo.front + key_fifo.count) % FIFO_SIZE;
	key_fifo.data[write_index] = b;
	if (key_fifo.count < FIFO_SIZE) {
		++key_fifo.count;
	} else {
		key_fifo.front = (key_fifo.front + 1) % FIFO_SIZE;
	}
}

static uint8_t keyboard_fifo_pop(void) {
	uint8_t value = KEY_ERROR_UNDEFINED;
	if (key_fifo.count) {
		value = key_fifo.data[key_fifo.front];
		key_fifo.front = (key_fifo.front + 1) % FIFO_SIZE;
		--key_fifo.count;
	}
	return value;
}

void keyboard_poll(void) {
	if (system_millis < deadline_for_key) {
		if (usb_send_keys_if_changed()) {
			deadline_for_key = 0;
		}
	}
	// TODO: Reset current endpoint buffer if OS isn't reading it.
	if (system_millis >= deadline_for_key && key_fifo.count) {
		uint8_t key = keyboard_fifo_pop();
		if (key == KEY_RESET) {
			usb_keyboard_keys_up();
		} else {
			usb_keyboard_key_down(key);
		}
		if (!usb_send_keys_if_changed()) {
			deadline_for_key = system_millis + KEY_TIMEOUT_MS;
		}
	}
}

static void push_shift_key_press(uint8_t code) {
	keyboard_fifo_push(KEY_SHIFT);
	keyboard_fifo_push(code);
	keyboard_fifo_push(KEY_RESET);
}

static void push_key_press(uint8_t code) {
	keyboard_fifo_push(code);
	keyboard_fifo_push(KEY_RESET);
}

static void push_char_to_fifo(char c) {
	if (c >= 'a' && c <= 'z') {
		push_key_press(KEY_A + (c - 'a'));
		return;
	}
	if (c >= 'A' && c <= 'Z') {
		push_shift_key_press(KEY_A + (c - 'A'));
		return;
	}
	if (c >= '1' && c <= '9') {
		push_key_press(KEY_1 + (c - '1'));
		return;
	}

	switch (c) {
	case ' ': push_key_press(KEY_SPACE); break;
	case '!': push_shift_key_press(KEY_1); break;
	case '"': push_shift_key_press(KEY_QUOTE); break;
	case '#': push_shift_key_press(KEY_3); break;
	case '$': push_shift_key_press(KEY_4); break;
	case '%': push_shift_key_press(KEY_5); break;
	case '&': push_shift_key_press(KEY_7); break;
	case '\'': push_key_press(KEY_QUOTE); break;
	case '(': push_shift_key_press(KEY_9); break;
	case ')': push_shift_key_press(KEY_0); break;
	case '*': push_shift_key_press(KEY_8); break;
	case '+': push_shift_key_press(KEY_EQUALS); break;
	case ',': push_key_press(KEY_COMMA); break;
	case '-': push_key_press(KEY_MINUS); break;
	case '.': push_key_press(KEY_PERIOD); break;
	case '/': push_key_press(KEY_SLASH); break;
	case '0': push_key_press(KEY_0); break;
	case ':': push_shift_key_press(KEY_SEMICOLON); break;
	case ';': push_key_press(KEY_SEMICOLON); break;
	case '<': push_shift_key_press(KEY_COMMA); break;
	case '=': push_key_press(KEY_EQUALS); break;
	case '>': push_shift_key_press(KEY_PERIOD); break;
	case '?': push_shift_key_press(KEY_SLASH); break;
	case '@': push_shift_key_press(KEY_2); break;
	case '[': push_key_press(KEY_LEFT_BRACE); break;
	case '\\': push_key_press(KEY_BACKSLASH); break;
	case ']': push_key_press(KEY_RIGHT_BRACE); break;
	case '^': push_shift_key_press(KEY_6); break;
	case '_': push_shift_key_press(KEY_MINUS); break;
	case '`': push_key_press(KEY_BACKQUOTE); break;
	case '{': push_shift_key_press(KEY_LEFT_BRACE); break;
	case '|': push_shift_key_press(KEY_BACKSLASH); break;
	case '}': push_shift_key_press(KEY_RIGHT_BRACE); break;
	case '~': push_shift_key_press(KEY_BACKQUOTE); break;
	}
}

void keyboard_type_string(const char const * str) {
	const char *c = str;
	while (*c != 0) {
		push_char_to_fifo(*c);
		++c;
	}
}
