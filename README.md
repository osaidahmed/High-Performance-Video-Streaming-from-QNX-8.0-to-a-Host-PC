# camera_example1_callback

This example source code shows how you can interact with the sensor service using libcamapi. Sensor service interfaces with camera drivers to send user applications camera frames through libcamapi.

camera_example1_callback shows an example of how to use the callbackmode; every time there is a new buffer available, `processCameraData` gets called.

### How to build

QNX SDP 8.0 is required. SDP and required packages can be installed with QNX Software Center.

Required packages:

- com.qnx.qnx800.target.sf.camapi
- com.qnx.qnx800.target.sf.base
- com.qnx.qnx800.target.mm.aoi
- com.qnx.qnx800.target.mm.mmf.core

Run the following commands to build the application:
```bash
# Source your QNX SDP script
source ~/qnx800/qnxsdp-env.sh

# Clone the repository
git clone https://gitlab.com/qnx/sample-apps/camera_example1_callback.git && cd camera_example1_callback

# Build and install
make install
```

### How to run

```bash
# scp libraries and tests to the target (note, mDNS is configured from
# /boot/qnx_config.txt and uses qnxpi.local by default).
TARGET_HOST=<target-ip-address-or-hostname>

# scp the built binary over to your QNX target
scp ./nto/aarch64/o.le/camera_example1_callback qnxuser@$TARGET_HOST:/data/home/qnxuser/bin

# ssh into the target
ssh qnxuser@$TARGET_HOST

# Make sure sensor service is running
# Run "pidin ar | grep sensor" to see if sensor is running, if not run this as root:
# su
# sensor -U 521:521,1001 -b external -r /data/share/sensor -c /system/etc/system/config/camera_module3.conf

# Run example; -u 1 means we want to use CAMERA_UNIT_1 which is specified in sensor_demo.conf
camera_example1_callback -u 1
```

You will see the following output:
```console
Channel averages: 84.772, 130.876, 129.243 took 3.472 ms (press any key to stop example)
```
