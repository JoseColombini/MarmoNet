# MarmoNet
MarmoNet is a biotelemetry system designed to track and monitor small marmosets that lives in urban environments. It was specifically designed to study the behavior and life model of the animals in Instituto Butantan and USP Campus. This project is part of a bigger project we named "Marmosets of Butantã" (free translation from "Saguis do Butantã"). It aims to enhance the knowledge of urban marmoset behavior using classical methods, Citizen Science and Biotelemetry. Please follow our [Instagram](https://www.instagram.com/saguisdobutantan/)

Marmosets are highly inteligent animals ([recently we discovered they can name themselves](https://www.theguardian.com/science/article/2024/aug/29/marmosets-behaviour-specific-names-study)), and have a great adaptability. Thus, they are ommonly found in city parks and live near people, so understanding their ecology and behavior in these new environments is a must to design healthier cities.

Although it targets  a specific set of animals we hope its design is general enough to be reused as a starting point for other biotelemetry system projects.

The system was developed using [Pulga](https://wiki.caninosloucos.org/index.php/Pulga), a Brazilian MCU, projected and designed at USP. We used [pulga-riot](https://github.com/caninos-loucos/pulga-riot), based on the original [RiotOS](https://github.com/RIOT-OS/RIOT).W We plan to migrate the system to Zephyr in a near future. 

# The Fundamentals of MarmoNet

MarmoNet is Wireless Sensor Network (WSN), thus it is designed with internal nodes (devices that collect data), Bases Stations (BS), that aggregate data from the nodes, and a sink device, which will aggregate all the data from all the BSs. As you can see in the Figure, we have an internal region monitored by the Nodes, and the border is delimited by the BSs (a logical, not a physical, border), and  the data will be sent to the Aggregator, allowing the biologist to study them.

IMAGE TBD

Since marmosets are smart it has a very low rate of recapture, so we cannot afford a system requiring battery changes or manual data recovery. This results in the described topology and a low-energy-consumption system. To reduce energy usage the system is system is synchronized, and every device wakes up at the same time to collect data and transfer it. It is a BS responsibility to resynchronize the nodes it finds, and it is the Aggregator role to sync the BSs.

MarmoNet is designed to solve the problem of use of resources in a spacial-temporal analysis, also aiming to understand the social behavior of the marmosets. Thus the data that it can collect are:

- RFID: each Node can detected other nodes IDs and save when it happened;
- Barometer: measure air pressure,  allowing inference of the animal’s height;
- Temperature;
- Relative Air Humidity (*soon, it still has a bug*);
- Light Sensor (TBD);


The MarmoNet can be configured to hace different behaviors (duty-cycles, which sensor will be used, etc) using [**marmonet_params.h**](), which holds the parameters to customize the network. And the [**marmonet_structs.h**]() holds how the data is structured in the network. Allowing for correct recovery.


## Node

The Node is the device that will be attached to the marmoset. It is a timed routine that will wakeup the Node every WAKEUP_PERIOD. When it happens, the Node will collect the data from the sensors and the IDs of the nearby Nodes. Functioning as an RFID/proximity tag.

We use BLE and it will use a broadcast topology, reducing communication overhead between nodes, and, thus, reducing energy consumption. The broadcast will cast the ID of the marmoset and the status of the Node (FailSafe or Std) with a TDMA, allowing all nodes to communicate with nearby nodes.

It can be connected to a BS so it will send all the data collected to the BS, and be resynced.

![Node ASM](./assets/fig/ASM_Node.png)

As described by the image the Node will use bluetooth to advertise its ID and status (FailSafe or Std). This will work 

## BS

The BS is responsible to collect the data from the Node, sync the Nodes and activate/deactivate sensors.

![BS ASM](./assets/fig/ASM_BS.png)


## Others elements

To attach the node to the animal it was developed a collar:

![BS ASM](./assets/fig/ASM_BS.png)

We hope that soon we will develop an app to collect the data from the BS and save it in the database :)



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





# Information
Engineering Final Project at Escola Politecnica - USP

Project Name: MarmoNet: a biotelemetry system for urban marmosets of the genus *Callithrix*

Author: Jose Colombini

Supervisor: Prof. Bruno Albertini (Engineer) and Dr. Erika Hings-Zaher (Biologist)

Collaborators: Louise Schineider (MSc. biology), Isabella Schrotter (BSc. Biology)

[Full Text can be found here: MarmoNet]()

To cite this work please use:

```
TODO
```
