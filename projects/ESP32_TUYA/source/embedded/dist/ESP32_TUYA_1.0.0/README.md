# ESP32_TUYA 1.0.0 — firmware và sơ đồ flash

Gói này dành cho firmware TuyaOpen của ESP32 classic (`ESP32-D0WD-V3` đã được ghi nhận trong lần nạp trước). Các địa chỉ dưới đây được đọc từ `partition-table.bin` và đối chiếu với nội dung ảnh QIO trong gói này.

## Cách nạp khuyến nghị: ảnh QIO ghép

Cài `esptool` nếu máy chưa có, kết nối ESP32 bằng cáp USB dữ liệu và kiểm tra cổng COM trong Device Manager. Mở PowerShell ngay tại thư mục chứa các file `.bin`, đóng Serial Monitor đang giữ cổng, rồi chạy lệnh sau (đổi `COM4` thành cổng đang dùng):

```powershell
esptool --chip esp32 --port COM4 write_flash 0x0000 ESP32_TUYA_QIO_1.0.0.bin
```

`ESP32_TUYA_QIO_1.0.0.bin` là ảnh ghép bắt đầu tại địa chỉ flash `0x0000`. Ảnh này đã chứa các thành phần cần thiết ở đúng offset trong bảng bên dưới. **Chỉ nạp ảnh QIO một lần ở `0x0000`; không nạp nó ở `0x10000` và không ghép thêm các file rời vào cùng lệnh.**

Thao tác ghi firmware sẽ thay đổi nội dung flash trên bo. Xác nhận đúng chip, cổng COM và file trước khi chạy. Nếu cần giữ dữ liệu đã lưu trên thiết bị, hãy sao lưu theo quy trình phù hợp trước khi nạp.

## Cách nạp từng file

Có thể dùng các ảnh rời thay cho ảnh QIO ghép. Chạy lệnh một dòng này từ thư mục gói và đổi `COM4` thành cổng thực tế:

```powershell
esptool --chip esp32 --port COM4 write_flash 0x1000 bootloader.bin 0x8000 partition-table.bin 0xD000 ota_data_initial.bin 0x10000 ESP32_TUYA.bin
```

Trong cách này, `ESP32_TUYA.bin` là ảnh ứng dụng, cần ghi tại `0x10000`. Không đặt ảnh ứng dụng ở `0x0000`. Các file `ESP32_TUYA_UA_1.0.0.bin` và `ESP32_TUYA_UG_1.0.0.bin` trong gói cũng được nhận dạng là ảnh ứng dụng ESP32; dùng một ảnh ứng dụng phù hợp tại `0x10000`, không dùng chung với ảnh QIO ghép.

Nếu `esptool` chưa có trong PATH, cài Espressif esptool theo hướng dẫn chính thức rồi mở terminal đã nhận lệnh `esptool`. Xem các tùy chọn được hỗ trợ bằng:

```powershell
esptool write_flash --help
```

## Bảng phân vùng

| Phân vùng | Loại | Địa chỉ bắt đầu | Kích thước | Ghi chú |
|---|---:|---:|---:|---|
| NVS | data | `0x9000` | `0x4000` (16 KiB) | Dữ liệu cấu hình không bay hơi |
| `otadata` | data | `0xD000` | `0x2000` (8 KiB) | Trạng thái OTA; gói có `ota_data_initial.bin` |
| `phy_init` | data | `0xF000` | `0x1000` (4 KiB) | Dữ liệu khởi tạo PHY |
| `ota_0` | app | `0x10000` | `0x180000` (1.5 MiB) | Slot ứng dụng OTA thứ nhất |
| `ota_1` | app | `0x190000` | `0x180000` (1.5 MiB) | Slot ứng dụng OTA thứ hai |
| `tuya` | data | `0x3B0000` | `0x28000` (160 KiB) | Vùng dữ liệu Tuya |
| `factory_nvs` | data | `0x3E0000` | `0x4000` (16 KiB) | NVS nhà máy |

Các vùng hệ thống được đặt ngoài bảng phân vùng:

| Thành phần | Địa chỉ flash | File |
|---|---:|---|
| Bootloader | `0x1000` | `bootloader.bin` |
| Partition table | `0x8000` | `partition-table.bin` |
| OTA initial data | `0xD000` | `ota_data_initial.bin` |
| Ứng dụng | `0x10000` | `ESP32_TUYA.bin` hoặc ảnh ứng dụng UA/UG |

## Nội dung gói

- `ESP32_TUYA_QIO_1.0.0.bin`: ảnh ghép để ghi tại `0x0000`.
- `bootloader.bin`, `partition-table.bin`, `ota_data_initial.bin`: thành phần riêng dùng với lệnh nạp nhiều file.
- `ESP32_TUYA.bin`, `ESP32_TUYA_UA_1.0.0.bin`, `ESP32_TUYA_UG_1.0.0.bin`: ảnh ứng dụng.
- `ESP32_TUYA_1.0.0.elf` và `.map`: file phục vụ debug/phân tích build; không truyền các file này cho lệnh flash ở trên.

Nếu có sẵn TuyaOpen CLI và mã nguồn, cách build/nạp theo cấu hình dự án nằm trong README của dự án `ESP32_TUYA`.
