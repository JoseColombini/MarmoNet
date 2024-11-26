# MarmoNet
MarmoNet is a biotelemetry system designed to track and monitor small marmosets that lives in urban enviroment. It was specifically designed to research the behavior and life model of the animals in Instituto Butantan and USP Campus.

Although it target a specific set of animals we hope its design is general enough to be reused as a starting point to another project.

The system was developed with a [Pulga](https://wiki.caninosloucos.org/index.php/Pulga), a Brazilian, using [pulga-riot](https://github.com/caninos-loucos/pulga-riot), based on the original [RiotOS](https://github.com/RIOT-OS/RIOT) (Hope it will be realesed without the constraints of this personalized Pulga Riot ASAP)

# Installation

**We strongly recommend you to read all the installation process before start**

To test this software please install all the [depences of RiotOS](https://doc.riot-os.org/getting-started.html) to your OS and git clone [pulga-riot](https://github.com/caninos-loucos/pulga-riot):

```
sudo apt update
sudo apt install git gcc-arm-none-eabi make gcc-multilib libstdc++-arm-none-eabi-newlib openocd gdb-multiarch doxygen wget unzip python3-serial
git clone https://github.com/caninos-loucos/pulga-riot.git
```

You will need to place the MarmoNet repo inside the **Examples/** folder of pulga-riot. It can be done using: 

```
cd pulga-riot/examples/
git clone https://github.com/JoseColombini/MarmoNet.git
```
Once you done it you can compile the code by just going to the directory of the part you want to build and using the command
```
make
```
All the configuration to compile is built in the make file, but the flashing process is not, if you want to flash it please use the appropriate tool, [please install JLink](https://www.segger.com/downloads/jlink/)

*Linux* users just use the command 
```
make PORT=/dev/ttyUSB0  flash term
```
*Windows* users use the **JFlashLight** from JLink directory, configure it to teh Device **NRF52840_XXAA** and Interface **SWD** with **4000 kHz**

Worth to remember this is specific for Pulga. Any of the compatible devices from this version of riot can be used but following the appropriate Board configuration commands (more information in [RiotOS page](https://doc.riot-os.org/getting-started.html))


# The Fundamentals of MarmoNet

It is composed of two devices *Node* and *Base Station*. The two of them share a configuration file called [**marmonet_params.h**](), which holds the parameters to personalized the network. And a [**marmonet_structs.h**](), the later holds the basics structs of the project.

## Node

The Node is the device that will be attached to the marmoset. It is a timed routine that will wakeup the Node every WAKEUP_PERIOD. When it happens, the Node will collect the data from the sensors and the IDs of the nearby Nodes.

**INSERT A FIGURE HERE**

## BS

The BS is responsible to collect the data from the Node, but also sync the Node.

**INSERT A FIGURE HERE**


## Others elements

To attached the Node to the animal it was developed a collar.

We hope that soon we will have the app to collect the data from the BS and save it in the database


# Information
Engineering Final Project at Escola Politecnica - USP

Project Name: MarmoNet: a biotelemetry system for urban marmosets of the genus *Callithrix*

Author: Jose Colombini

Supervisor: Bruno Albertini (Engineer) and Erika Hings-Zaher (Biologist)

[Full Text can be found here: MarmoNet]()

To cite this work please use:

```
TODO
```
