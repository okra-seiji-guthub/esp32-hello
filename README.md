# esp32-hello

## install BME280 driver

```bash
pio pkg search BME280
pio pkg install --library "7semi-solutions/7Semi BME280"

## Upload & run
pio run -t upload

## Upload & run with monitoring
pio run -t upload -t monitor

pio run --target clean
```
