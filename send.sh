#!/bin/bash
cat $1 | base64 > b64_transfer
python send.py b64_transfer
rm b64_transfer