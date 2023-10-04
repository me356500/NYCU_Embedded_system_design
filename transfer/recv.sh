#!/bin/bash
./recv b64_transfer
base64 -d b64_transfer > $1
rm b64_transfer