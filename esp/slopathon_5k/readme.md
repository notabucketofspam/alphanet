# slopathon_5k

This is made for one of the slightly-older Espressif ESP32 models (ESP32-D0WDQ6).

If you have a newer chip, you might want to tweak the source code a touch.

---

[Here's a convenient download link for the Espressif Flash Tool](https://github.com/notabucketofspam/get-espressif-working/blob/master/flashtool/flash_download_tool.zip)

Here's the offset for each of the `.bin` files that you need:

```
0x1000  "bootloader/bootloader.bin"
0x8000  "partition_table/partition-table.bin"
0x10000 "slopathon_5k.bin"
```

All of those `.bin` files are inside of [the `flash_me` folder](./flash_me/)

Some other flash settings:
- SPI SPEED: **40MHz**
- SPI MODE: **DIO**
