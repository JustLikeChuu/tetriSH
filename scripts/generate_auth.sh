#!/usr/bin/env bash
# Generates a local RSA private key + self-signed cert for bombd into auth/.
#
# The client only verifies the server via proof-of-possession (the server
# signs a nonce with the private key matching the cert it sent -
# verify_message_pss() in corestack/src/lib/libbattleroyale/client.c). It
# never chains the cert to a CA, so a self-signed cert is sufficient here.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
AUTH_DIR="$ROOT_DIR/auth"
KEY_FILE="$AUTH_DIR/private_key.pem"
CERT_FILE="$AUTH_DIR/server_signed.crt"
RSA_KEY_BITS=1024 # must match RSA_KEY_BITS in corestack/src/lib/libbattleroyale/crypto.h

mkdir -p "$AUTH_DIR"

if [[ -f "$KEY_FILE" && -f "$CERT_FILE" ]]; then
    echo "[generate_auth] $KEY_FILE and $CERT_FILE already exist, leaving them as-is."
    echo "[generate_auth] delete them first (or run with FORCE=1) to regenerate."
    exit 0
fi

if [[ "${FORCE:-0}" != "1" && ( -f "$KEY_FILE" || -f "$CERT_FILE" ) ]]; then
    echo "[generate_auth] one of $KEY_FILE / $CERT_FILE already exists. Set FORCE=1 to overwrite both." >&2
    exit 1
fi

openssl req -x509 -newkey "rsa:$RSA_KEY_BITS" -keyout "$KEY_FILE" -out "$CERT_FILE" \
    -days 365 -nodes -sha256 \
    -subj "/C=SG/ST=Singapore/L=Singapore/O=corestack-dev/CN=localhost"

chmod 600 "$KEY_FILE"

echo "[generate_auth] wrote $KEY_FILE and $CERT_FILE"
