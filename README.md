# Description

This repository contains the **DPS310** pressure and temperature sensor driver.

# Dependencies

The driver relies on:

* An external `types.h` header file defining the **standard C types** of the targeted MCU.
* The **embedded utility functions** defined in the [embedded-utils](https://github.com/Ludovic-Lesur/embedded-utils) repository.

Here is the versions compatibility table:

| **dps310-driver** | **embedded-utils** |
|:---:|:---:|
| [sw1.1](https://github.com/Ludovic-Lesur/dps310-driver/releases/tag/sw1.1) | >= [sw2.0](https://github.com/Ludovic-Lesur/embedded-utils/releases/tag/sw2.0) |
| [sw1.0](https://github.com/Ludovic-Lesur/dps310-driver/releases/tag/sw1.0) | [sw1.3](https://github.com/Ludovic-Lesur/embedded-utils/releases/tag/sw1.3) to [sw1.4](https://github.com/Ludovic-Lesur/embedded-utils/releases/tag/sw1.4) |

# Compilation flags

| **Flag name** | **Value** | **Description** |
|:---:|:---:|:---:|
| `DPS310_DRIVER_DISABLE_FLAGS_FILE` | `defined` / `undefined` | Disable the `dps310_driver_flags.h` header file inclusion when compilation flags are given in the project settings or by command line. |
| `DPS310_DRIVER_I2C_ERROR_BASE_LAST` | `<value>` | Last error base of the low level I2C driver. |
| `DPS310_DRIVER_DELAY_ERROR_BASE_LAST` | `<value>` | Last error base of the low level delay driver. |
