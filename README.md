# nrf-mesh
Mesh network for the nRF24L01+ module

# How the Application should use the Mesh library
<img src="https://github.com/wassfila/media/blob/master/STM32Libs_Design/Application_Interface.png" height=400>

# Components dependencies
In the diagram below, the Serial port is declared by the main application, and provided as parameter for the Mesh, that also forward it to the nRF component. The SPI pins are provided as constructor parameters for the Mesh that forwards them to the nRF constructor.

<img src="https://github.com/wassfila/media/blob/master/STM32Libs_Design/Components_Dependencies.png" height=400>
