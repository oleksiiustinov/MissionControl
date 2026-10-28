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
#include "stadia_ble_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    namespace {

        // Report id the payload is re-framed with; HOGP strips it and it's only used to pick the decoder below
        constexpr u8 InputReportId = 0x03;

        constexpr u8 TriggerMax = 0xff;

    }

    Result StadiaBleController::HandleInputReport(const u8 *data, size_t size) {
        // The pad may notify on other input Reports too (e.g. consumer keys); only the gamepad report is this long
        R_UNLESS(size >= sizeof(StadiaInputReport0x03), svc::ResultInvalidSize());
        R_UNLESS(hos::GetVersion() >= hos::Version_9_0_0, sf::ResultNotSupported());

        auto &event = m_event_info.data_report.v9;
        R_UNLESS(size + 1 <= sizeof(event.report.data), svc::ResultInvalidSize());

        event.res = 0;
        event.proto_mode = 0;
        event.addr = m_address;
        event.pad = 0;
        event.report.size = size + 1;
        event.report.data[0] = InputReportId;
        std::memcpy(&event.report.data[1], data, size);

        R_RETURN(this->HandleDataReportEvent(&m_event_info));
    }

    Result StadiaBleController::SendNeutralReport() {
        const StadiaInputReport0x03 neutral = {
            .dpad = StadiaDPad_Released,
            .left_stick  = { 0x80, 0x80 },
            .right_stick = { 0x80, 0x80 },
        };
        R_RETURN(this->HandleInputReport(reinterpret_cast<const u8 *>(&neutral), sizeof(neutral)));
    }

    void StadiaBleController::ProcessInputData(const bluetooth::HidReport *report) {
        if ((report->data[0] != InputReportId) || (report->size < sizeof(StadiaInputReport0x03) + 1)) {
            return;
        }

        auto src = reinterpret_cast<const StadiaInputReport0x03 *>(&report->data[1]);

        m_left_stick  = PackAnalogStickValues(src->left_stick.x,  InvertAnalogStickValue(src->left_stick.y));
        m_right_stick = PackAnalogStickValues(src->right_stick.x, InvertAnalogStickValue(src->right_stick.y));

        m_buttons.dpad_up    = (src->dpad == StadiaDPad_N) || (src->dpad == StadiaDPad_NE) || (src->dpad == StadiaDPad_NW);
        m_buttons.dpad_right = (src->dpad == StadiaDPad_E) || (src->dpad == StadiaDPad_NE) || (src->dpad == StadiaDPad_SE);
        m_buttons.dpad_down  = (src->dpad == StadiaDPad_S) || (src->dpad == StadiaDPad_SE) || (src->dpad == StadiaDPad_SW);
        m_buttons.dpad_left  = (src->dpad == StadiaDPad_W) || (src->dpad == StadiaDPad_NW) || (src->dpad == StadiaDPad_SW);

        // Mapped by position, like the other Xbox-layout pads: Stadia A (bottom) is the Switch's B, and so on
        m_buttons.A = src->B;
        m_buttons.B = src->A;
        m_buttons.X = src->Y;
        m_buttons.Y = src->X;

        m_buttons.L  = src->L1;
        m_buttons.R  = src->R1;
        m_buttons.ZL = src->left_trigger  > (m_trigger_threshold * TriggerMax);
        m_buttons.ZR = src->right_trigger > (m_trigger_threshold * TriggerMax);

        m_buttons.minus = src->options;
        m_buttons.plus  = src->menu;

        m_buttons.lstick_press = src->L3;
        m_buttons.rstick_press = src->R3;

        m_buttons.home    = src->stadia;
        m_buttons.capture = src->capture;
    }

    Result StadiaBleController::SetVibration(const SwitchMotorData *motor_data) {
        const u16 strong = static_cast<u16>(UINT16_MAX * std::clamp(std::max(motor_data->left_motor.low_band_amp,  motor_data->right_motor.low_band_amp),  0.0f, 1.0f));
        const u16 weak   = static_cast<u16>(UINT16_MAX * std::clamp(std::max(motor_data->left_motor.high_band_amp, motor_data->right_motor.high_band_amp), 0.0f, 1.0f));
        R_RETURN(this->WriteRumble(strong, weak));
    }

    Result StadiaBleController::CancelVibration() {
        R_RETURN(this->WriteRumble(0, 0));
    }

    Result StadiaBleController::WriteRumble(u16 strong, u16 weak) {
        R_SUCCEED_IF(!m_has_output_report);

        // hid sends rumble at ~60Hz; a GATT write for each would saturate the LE link, so only send changes
        R_SUCCEED_IF((strong == m_last_rumble_strong) && (weak == m_last_rumble_weak));
        m_last_rumble_strong = strong;
        m_last_rumble_weak = weak;

        const StadiaOutputReport0x05 report = { .strong = strong, .weak = weak };
        R_RETURN(btdrvWriteGattCharacteristic(m_conn_id, m_service_primary, &m_service_id, &m_output_report_id, &report, sizeof(report), BtdrvGattAuthReqType_NoMitm, false));
    }

}
