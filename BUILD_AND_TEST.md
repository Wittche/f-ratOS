# ISO Oluşturma ve Test Talimatları

## SORUN: ISO dosyası oluşturulmamış!

`build/auroraos.iso` dosyası yok. Bootable ISO oluşturman gerekiyor.

## Çözüm:

```bash
cd ~/f-ratOS  # veya projenin olduğu klasör

# 1. Kernel'i build et
make clean
make kernel

# 2. ISO'yu oluştur (grub-mkrescue gerekli!)
make iso

# 3. ISO'yu test et
make run-iso
# VEYA kendi bilgisayarında:
qemu-system-x86_64 -cdrom build/auroraos.iso -m 256M
```

## Eğer `grub-mkrescue` bulunamazsa:

```bash
sudo apt-get install grub-pc-bin grub-common xorriso mtools
```

Sonra tekrar `make iso` yap.

## Debug için:

Ekranda şu harfleri göreceksin:
- A, B, C, D... = Boot adımları
- Hangi harfe kadar geldiğini söyle!
