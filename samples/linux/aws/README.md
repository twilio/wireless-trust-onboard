## Hardware requirements

1. Raspberry Pi
2. [Centurion Gemalto Cinterion concept board](https://www.gemalto.com/m2m/development/cinterion-concept-board)

## Set up
1. Update line 30 in `aws_iot_config.h` to point to your AWS account

## How to run
Type the following in a Terminal:
```
make
./start-modem.sh
./subscribe_publish_sample_cpp
```
