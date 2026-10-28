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
#include "emulated_switch_controller.hpp"
#include "ble_gamepad.hpp"

namespace ams::controller {

    // Google Stadia controller hat switch values (8 = released)
    enum StadiaDPadDirection : u8 {
        StadiaDPad_N,
        StadiaDPad_NE,
        StadiaDPad_E,
        StadiaDPad_SE,
        StadiaDPad_S,
        StadiaDPad_SW,
        StadiaDPad_W,
        StadiaDPad_NW,
        StadiaDPad_Released
    };

    // Layout as documented by SDL's hidapi Stadia driver (USB report 0x03), report id excluded
    struct StadiaInputReport0x03 {
        u8 dpad;

        u8 capture   : 1;
        u8 assistant : 1;
        u8           : 2;
        u8 stadia    : 1;
        u8 menu      : 1;
        u8 options   : 1;
        u8 R3        : 1;

        u8 L3        : 1;
        u8 R1        : 1;
        u8 L1        : 1;
        u8 Y         : 1;
        u8 X         : 1;
        u8 B         : 1;
        u8 A         : 1;
        u8           : 1;

        AnalogStick<u8> left_stick;
        AnalogStick<u8> right_stick;
        u8 left_trigger;
        u8 right_trigger;
    } PACKED;
    static_assert(sizeof(StadiaInputReport0x03) == 9);

    // Rumble, output report 0x05, report id excluded
    struct StadiaOutputReport0x05 {
        u16 strong;
        u16 weak;
    } PACKED;

    /*
     * Google Stadia controller in Bluetooth mode (after Google's Bluetooth unlock), which only speaks Bluetooth LE.
     */
    class StadiaBleController : public EmulatedSwitchController, public BleGamepad {

        public:
            static constexpr HardwareID StadiaHardwareId = {0x18d1, 0x9400};

            StadiaBleController(bluetooth::Address address, u32 conn_id, const BtdrvGattId &service_id, bool service_primary, const BtdrvGattId &output_report_id, bool has_output_report)
            : EmulatedSwitchController(address, StadiaHardwareId)
            , m_conn_id(conn_id)
            , m_service_id(service_id)
            , m_service_primary(service_primary)
            , m_output_report_id(output_report_id)
            , m_has_output_report(has_output_report) { }

            Result HandleInputReport(const u8 *data, size_t size) override;
            Result SendNeutralReport() override;

        protected:
            void ProcessInputData(const bluetooth::HidReport *report) override;
            Result SetVibration(const SwitchMotorData *motor_data) override;
            Result CancelVibration() override;

        private:
            Result WriteRumble(u16 strong, u16 weak);

            u32 m_conn_id;
            BtdrvGattId m_service_id;
            bool m_service_primary;
            BtdrvGattId m_output_report_id;
            bool m_has_output_report;

            u16 m_last_rumble_strong = 0;
            u16 m_last_rumble_weak = 0;

            bluetooth::HidReportEventInfo m_event_info;
    };

}
