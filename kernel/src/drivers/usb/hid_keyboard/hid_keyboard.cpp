#include "hid_keyboard.h"
#include "drivers/usb/hidparse.h"


static int hid_keyboard_init(generic_usb_controller_t controller, uint8_t deviceAddress) {
    usb_device_req_packet_t packet = {
        .request_type = usb::REQUEST_TYPE_DESCRIPTOR,
        .request = usb::REQUEST_CODE_GET_DESCRIPTOR,
        .value = usb::DESCRIPTOR_TYPE_CONFIG,
        .index = 0,
        .length = 0
    };

    usb_buffer_t buffer;
    if(!controller.send_request(controller.real_device, &packet, deviceAddress, &buffer)) {
        return -1;
    }

    usb_config_desc_t* config = (usb_config_desc_t *)buffer.buf;
    usb_hid_desc_t* hid = (usb_hid_desc_t *)usb_find_descriptor_type(config, usb::DESCRIPTOR_RET_TYPE_HID);
    if(!hid) {
        usb_buffer_free(&buffer);
        return -2;
    }

    int report_length = hid->descriptors[0].desc_length;
    packet.request_type = usb::REQUEST_TYPE_INT_STAT;
    packet.value = usb::DESCRIPTOR_TYPE_HID_REPORT;
    packet.length = report_length;

    usb_buffer_free(&buffer);
    if(!controller.send_request(controller.real_device, &packet, deviceAddress, &buffer)) {
        return -1;
    }

    HidParser parser(buffer.buf, buffer.size);
    parser.parse();
}

USB_MODULE_INIT(HID_KBD_USB_MODULE, hid_keyboard_init, 0x3, 0x1, 0x1);