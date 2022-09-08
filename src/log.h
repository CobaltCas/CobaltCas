#pragma once

namespace Log {

    void logout(const char *fmt, ...);
    void logout_command_dump(const void *p, uint16_t size);
    void logout_dump(const void *p, uint16_t size, uint16_t tabCount);
    void logout_bitmap(const uint8_t *bitmap, uint16_t size, uint16_t spaceCount);
    void logout_timestamp(void);
    void logout_send_raw_data(const void *data, uint16_t size);
    void logout_receive_raw_data(const void *data, uint16_t size);
    void logout_address_name(uint16_t address);
}
