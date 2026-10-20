#!/usr/bin/env bash
echo "Launching NwOS (GUI mode)..."
qemu-system-x86_64 -cdrom NwOS-gui.iso -m 128M -nic user,model=e1000