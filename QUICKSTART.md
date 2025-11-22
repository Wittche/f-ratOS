# AuroraOS Quick Start Guide

Hızlı başlangıç için bu rehberi takip edin.

## 🚀 Tek Komutla Çalıştırma

Kernel'ı build edip test etmek için sadece:

```bash
make test
```

Veya her şeyi sıfırdan:

```bash
make quick
```

Bu kadar! 🎉

## 📋 Komut Listesi

### Basit Kullanım

| Komut | Ne Yapar |
|-------|----------|
| `make test` | Build + ISO oluştur + QEMU'da çalıştır (ÖNERİLEN) |
| `make quick` | Clean + build + ISO + çalıştır (en güvenli) |
| `make clean` | Build dosyalarını temizle |

### Detaylı Kullanım

| Komut | Ne Yapar |
|-------|----------|
| `make` veya `make all` | Bootloader ve kernel'ı build et |
| `make kernel` | Sadece kernel'ı build et |
| `make iso` | Bootable ISO oluştur (GRUB2) |
| `make run-iso` | ISO'dan QEMU'da çalıştır |
| `make run` | UEFI ile çalıştır (OVMF gerekli) |
| `make help` | Tüm komutları göster |

## 🔧 Gereksinimler

### Build için:
- GCC
- Clang
- GNU Binutils (as, ld, objcopy)
- GRUB (grub-mkrescue, grub-pc-bin, grub-efi-amd64-bin)
- xorriso

Ubuntu/Debian'da kurulum:
```bash
sudo apt install build-essential clang grub-pc-bin grub-efi-amd64-bin xorriso
```

### Test için:
- QEMU

```bash
sudo apt install qemu-system-x86
```

## 🐛 Sorun Giderme

### "grub-mkrescue not found" hatası
```bash
sudo apt install grub-pc-bin grub-efi-amd64-bin xorriso
```

### "qemu-system-x86_64 not found" hatası
```bash
sudo apt install qemu-system-x86
```

### Boot loop sorunu
Bu commit'te düzeltildi! `process_exit()` artık düzgün çalışıyor.

## 📝 Ne Değişti?

### Boot Loop Düzeltmesi
- ❌ Önceden: `exit()` çağrıldığında kernel reboot oluyordu
- ✅ Şimdi: `exit()` çağrıldığında kernel düzgün halt ediyor

### ISO Desteği
- ✅ GRUB2 ile bootable ISO oluşturma
- ✅ Tek komutla build ve çalıştırma
- ✅ Gerçek donanımda boot edebilme

## 🎯 Geliştirme Döngüsü

1. Kod değişikliği yap
2. `make test` çalıştır
3. QEMU'da test et
4. Ctrl+C ile çık
5. Tekrar et!

## 📖 Daha Fazla Bilgi

`make help` komutuyla tüm hedefleri görebilirsiniz.
