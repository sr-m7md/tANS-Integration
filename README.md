# tANS-Integration
My attempt to integrate a simplified tANS algorithm into the ContikiNG environment

## Table of contents
* [General info](#general-info)  
* [Final result](#final-result)  
* [Features](#features)  
* [Technologies](#technologies)  
* [Setup](#setup)  

---

## General info  
First, let's take a look at what tANS is and why it is important to be able to integrate it!
tANS (Tabled Asymmetric Numeral Systems) is a modern entropy compression method that combines the efficiency of arithmetic coding with the simplicity of Huffman implementation. It allows for high compression ratios with minimal computational and memory overhead, making it particularly valuable for embedded systems and IoT devices.

The integration of tANS into Contiki-NG, an operating system for the Internet of Things, opens up the possibility of reducing the amount of data transmitted over wireless channels and lowering energy consumption. This is critically important in the context of limited sensor node resources, where every byte transmitted and every processor cycle counts.

---

