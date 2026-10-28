/*
 * Copyright (c) 2020-2026 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::controller {

    /*
     * What the BLE host needs from a controller it presents to the system, whatever its make: input arrives as
     * HID-over-GATT notification payloads (report id already stripped by HOGP).
     */
    class BleGamepad {

        public:
            virtual ~BleGamepad() = default;

            // Payload of a notification from a HID input Report characteristic
            virtual Result HandleInputReport(const u8 *data, size_t size) = 0;

            // hid registers a controller on its first input report, and pads only report when touched, so the
            // host delivers a resting state (sticks centred, nothing pressed) right after attaching
            virtual Result SendNeutralReport() = 0;
    };

}
