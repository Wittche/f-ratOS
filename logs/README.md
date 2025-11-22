# QEMU Logs

Bu klasör QEMU çalıştırıldığında otomatik olarak oluşturulan log dosyalarını içerir.

## Log Dosyaları

Her QEMU çalıştırması için iki log dosyası oluşturulur:

1. **qemu_[mode]_[timestamp].log** - Serial port çıktısı (kernel konsol mesajları)
2. **qemu_debug_[timestamp].log** - QEMU debug çıktısı (CPU resets, guest errors, vs.)

## Log Modları

- `qemu_iso_*.log` - ISO image ile çalıştırma (`make run-iso`)
- `qemu_uefi_*.log` - UEFI mode ile çalıştırma (`make run`)
- `qemu_bios_*.log` - BIOS/Multiboot mode ile çalıştırma (`make run-bios`)

## Temizleme

Tüm log dosyalarını silmek için:
```bash
make clean-logs
```

## Timestamp Format

Log dosyaları şu formatta timestamp içerir: `YYYYMMDD_HHMMSS`

Örnek: `qemu_iso_20231122_143052.log` (22 Kasım 2023, 14:30:52)
