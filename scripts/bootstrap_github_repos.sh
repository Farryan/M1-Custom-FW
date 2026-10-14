#!/usr/bin/env bash
# Phase 1: Automated GitHub Repository Setup for https://github.com/Farryan
set -euo pipefail

GH_USER="Farryan"
MAIN_REPO="M1-Custom-FW"
C6_REPO="M1-ESP32C6-FW"

echo "[1/5] Verifying GitHub CLI authentication..."
gh auth status

echo "[2/5] Forking Monstatek/M1 -> ${GH_USER}/${MAIN_REPO}..."
gh repo fork Monstatek/M1 --fork-name "${MAIN_REPO}" --clone=true --default-branch-only || true

echo "[3/5] Creating companion ESP32-C6 repository ${GH_USER}/${C6_REPO}..."
gh repo create "${GH_USER}/${C6_REPO}" --public \
  --description "Companion ESP32-C6 SPI Coprocessor Firmware (ESP-IDF v5.x) for Monstatek M1" \
  --clone=false || true

echo "[4/5] Cloning shallow reference repositories into .reference/..."
mkdir -p .reference
git clone --depth 1 https://github.com/DarkFlippers/unleashed-firmware.git .reference/unleashed-firmware || true
git clone --depth 1 https://github.com/UberGuidoZ/Flipper.git .reference/UberGuidoZ-Flipper || true
git clone --depth 1 https://github.com/bedge117/esp32-at-monstatek-m1.git .reference/esp32-at-monstatek-m1 || true

echo "[5/5] Bootstrap complete."
