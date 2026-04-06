# CanSimulatorWithSniffer

To test the system locally with **vcan**, you must set up the virtual interface on your computer first.add vcan: 
### 1. Setup vcan
Add the virtual interface: 
```bash
sudo ip link add vcan0 type vcan
```
```bash 
sudo ip link set vcan0 up type vcan
```
### 2. Run the simulation
to test it compile the files and run them in separate terminals:
Terminal 1:
```bash
  ./bin/dump vcan0
``` 
Terminal 2:
```bash
./bin/send vcan0
``` 
 --------------

### To test it on hardware version:
```bash
sudo ip link set can0 up type can bitrate 125000
``` 

Terminal 1:
```bash
  ./bin/dump can0
``` 
Terminal 2:
```bash
./bin/send can0
``` 

## test canClient
Terminal 1:
```bash
  ./bin/canClient vcan0
``` 
Terminal 2:
```bash
./bin/send vcan0
``` 