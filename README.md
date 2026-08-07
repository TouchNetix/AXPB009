# AXPB009 - TNx Protocol Bridge #

# Recommended IDE #
This project works with the STM32CubeIDE. It can be downloaded [here](https://www.st.com/en/development-tools/stm32cubeide.html).

# Import Project #
Open STM32CubeIDE and create a workspace.

Once cloned to a local repository, the project can be imported to the IDE:

* Navigate to *File->Import*
* Select *Existing Projects into Workspace*
* Browse to where the project has been cloned to as the root directory
* Select the axpb009 project
* There should be no options set other than *Search for nested projects*
* Press finish

The project should appear in the project explorer window. Make sure all the files are present and that you can build the project.

# Build Targets #
Once imported, the code can be compiled to target multiple chips in the STM32F0 line of microcontrollers. To change the target:

* Right click on the open project AXPB009->Build Configurations->Set Active->(Build target)

The files created after compilation (.elf, .hex, .bin) are put into the relevant folder in the top level AXPB009 project folder - if the folder is missing it will create one automatically.

All targets can be built at the same time by clicking 'Build all' from the Build configuration menu. Each target can also be built individually if required.

# Docker/CMake Firmware Build #
Docker and CMake can be used to create reproducible release files with the pinned Arm GNU Toolchain 15.3.rel1:

```powershell
git submodule update --init --recursive
.\Tools\build_firmware_package.ps1
```

This builds the supported production targets `STM32F070CB` and `STM32F072CB`. The `STM32F042F6` target is excluded because the current firmware exceeds its 32 KiB flash capacity. Versioned HEX and DFU files are written to `FirmwarePackage/`.

# Pin Mappings #

Some of the build targets have slight variations on the pins used for certain functions. Each chip and its pin mappings has been listed below:

| Pin Function  | STM32F042F6        | STM32F070CB        | STM32F072CB        |
| ------------- | ------------------ | ------------------ | ------------------ |
| nRESET        | PB1                | PB1                | PB1                |
| nIRQ          | PA0                | PA0                | PA0                |
| USB_DM        | PA11               | PA11               | PA11               |
| USB_DP        | PA12               | PA12               | PA12               |
| SWDIO         | PA13               | PA13               | PA13               |
| SWCLK         | PA14               | PA14               | PA14               |
| COMMS_SELECT  | PA2                | PB2                | PB2                |
| SPI_nSS       | PA4                | PA4                | PA4                |
| SPI_SCK       | PA5                | PA5                | PA5                |
| SPI_MISO      | PA6                | PA6                | PA6                |
| SPI_MOSI      | PA7                | PA7                | PA7                |
| I2C_SCL       | PF1                | PB6                | PB6                |
| I2C_SDA       | PF0                | PB7                | PB7                |
| LED_USB       | PB8                | PB8                | PB8                |
| LED_AXIOM     | PA1                | PA1                | PA1                |


# Creating *.dfu files #
The Docker/CMake build creates versioned HEX and DFU release files automatically.

For a manual STM32CubeIDE build, STM32CubeProgrammer can use the generated .elf file directly. If using ST's DfuSe application, a .dfu file can be created with ST's DFU File Manager.
