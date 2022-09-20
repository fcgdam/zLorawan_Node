# Zephyr RTOS Lorawan Node

This is a sample project for a Class_a TTN Lorawan node.

The code was tested on a blackpill F411ce board and on a Black F407VE board.

Checkout the blog post at https://primalcortex.wordpress.com/2020/11/17/a-zephyr-rtos-based-ttn-lorawan-node/ for more information.

The latest code commit allows the code to compile/target the Zephyr RTOS 3.11.99 version.

# How to use it:

Before building, change on the src/main.c file the necessary TTN keys, specifically for the OTAA activation the DEV_EUI, the JOIN_EUI that on TTN dashboard is called APPLICATION_EUI and the APP_KEY. The keys are to be used in standard LSB format, so no change needed to be done on the TTN dasboard interface.

# Compiling and flashing

Install and setup Zephyr RTOS.

Build after configuring the keys with:  west build -b blackpill_f411ce -p

If using st-link use: west flash --runner openocd.

# Checking the result.

The onboard led should flash rapidly to give time to connect through serial console: screen /dev/ttyACM0 for example.
After the led is flashing slower, the Lorawan node part is running.

As it is now, the code should only send one data frame after the joining process.

