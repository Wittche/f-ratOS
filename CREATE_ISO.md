# AuroraOS Bootable ISO Oluşturma Kılavuzu

## Sorun: "No Bootable Device" Hatası

Bu hata GRUB'un bile yüklenmediği anlamına gelir. ISO'nun boot sektörü eksik.

## Çözüm 1: GRUB Araçlarını Kur (Önerilen)

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install grub-pc-bin grub-common xorriso mtools
```

### Arch Linux:
```bash
sudo pacman -S grub xorriso mtools
```

### Fedora/RHEL:
```bash
sudo dnf install grub2-tools-extra xorriso mtools
```

Sonra:
```bash
cd /path/to/Wittche
make iso
qemu-system-x86_64 -cdrom build/auroraos.iso -m 256M -serial stdio
```

## Çözüm 2: Manuel ISO Oluşturma (grub-mkrescue ile)

```bash
# 1. Kernel build
make clean && make kernel

# 2. ISO dizini hazırla
mkdir -p iso/boot/grub
cp build/kernel.elf iso/boot/kernel.elf

# 3. GRUB config zaten var (iso/boot/grub/grub.cfg)

# 4. Bootable ISO oluştur
grub-mkrescue -o build/auroraos.iso iso/

# 5. Test et
qemu-system-x86_64 -cdrom build/auroraos.iso -m 256M -serial stdio
```

## Çözüm 3: Direkt Kernel Test (ISO olmadan)

QEMU Multiboot2 desteğini kullan:

```bash
make clean && make kernel
qemu-system-x86_64 -kernel build/kernel.elf -m 256M -serial stdio
```

**Not:** QEMU 6.0+ gerekir, bazı eski versiyonlarda Multiboot2 desteği sınırlı.

## Çözüm 4: VirtualBox ile Test

```bash
# 1. ISO oluştur (grub-mkrescue ile)
make iso

# 2. VirtualBox VM oluştur
VBoxManage createvm --name "AuroraOS" --ostype "Linux_64" --register
VBoxManage modifyvm "AuroraOS" --memory 256 --cpus 1
VBoxManage storagectl "AuroraOS" --name "IDE" --add ide
VBoxManage storageattach "AuroraOS" --storagectl "IDE" --port 0 --device 0 \
    --type dvddrive --medium build/auroraos.iso

# 3. Başlat
VBoxManage startvm "AuroraOS"
```

## Sorun Giderme

### Hata: "grub-mkrescue: command not found"
- **Çözüm:** Yukarıdaki paket kurulum komutlarını çalıştır

### Hata: "error: cannot find a device for / (is /dev mounted?)"
- **Çözüm:** grub-pc-bin paketi eksik
```bash
sudo apt-get install grub-pc-bin
```

### Hata: "xorriso: command not found"
- **Çözüm:** xorriso paketi eksik
```bash
sudo apt-get install xorriso
```

### Hata: "mformat: command not found"
- **Çözüm:** mtools paketi eksik
```bash
sudo apt-get install mtools
```

### ISO oluştu ama "No bootable device"
- **Neden:** grub-mkrescue çalışmadı veya GRUB boot image'ı eklenemedi
- **Kontrol:**
```bash
# ISO içeriğini kontrol et
isoinfo -l -i build/auroraos.iso | grep -i boot
```
- **Çözüm:** Tüm GRUB paketlerini kur ve yeniden dene

## Debug: Kernel'in Multiboot2 Header'ını Kontrol Et

```bash
# Multiboot2 magic number kontrolü (0xE85250D6 olmalı)
od -A x -t x1z -N 16 build/kernel.bin

# Beklenen çıktı:
# 000000 d6 50 52 e8 00 00 00 00 50 00 00 00 da ae ad 17

# Entry point kontrolü
nm build/kernel.elf | grep _start
# Çıktı: 0000000000101000 T _start

# ELF program headers kontrolü
readelf -l build/kernel.elf | head -20
```

## Alternatif: USB Bootable Oluştur

```bash
# 1. ISO oluştur
make iso

# 2. USB'ye yaz (DİKKAT: /dev/sdX'i doğru device ile değiştir!)
sudo dd if=build/auroraos.iso of=/dev/sdX bs=4M status=progress
sudo sync
```

## Beklenen Çıktı (Başarılı Boot)

```
AuroraOS Kernel Starting...
[GDT] Initialized
[IDT] Initialized
[PMM] Initialized (Memory: XXXMB)
[VMM] Initialized
[Heap] Initialized
[Timer] Initialized (Freq: 100Hz)
[Keyboard] Initialized
[Scheduler] Initialized
[Syscalls] Initialized
[TSS] Initialized
[UserMode] Transitioning to user mode...
```

## Yardım

Sorun devam ederse:
1. QEMU ve GRUB versiyonlarını kontrol et:
```bash
qemu-system-x86_64 --version
grub-mkrescue --version
```

2. ISO'nun boyutunu kontrol et (en az 2-3 MB olmalı):
```bash
ls -lh build/auroraos.iso
```

3. Detaylı QEMU log:
```bash
qemu-system-x86_64 -cdrom build/auroraos.iso -m 256M -serial stdio \
    -d int,cpu_reset -D qemu.log
```
