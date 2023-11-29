# ProteiTestProject

## Dependencies
- GoogleTest
- spdlog
- cpp-httplib
- nlohmann-json
- curl(for example client usage)

## 1 step - build and install GTest framework
#### When you in /downloads/ or any else dir
```bash
git clone https://github.com/google/googletest.git
cd googletest && mkdir build && cd build && cmake .. && sudo make && sudo make install
```
## 2 step - build CallCenter project
#### When you in project directory:
```bash
mkdir build && cd build && cmake .. && sudo make
```
#### If you just want to perform tests:
```bash
sudo make test
```
## Usage
#### Run server app
```bash
./CallCenter
```
## Commands list(must be executed in other terminal(=client))
#### Make a call
```bash
curl -d "number=89315532090" -X POST http::/localhost:8080/call
```
#### Change a config file
```bash
curl -d "name=../cfg/defaultCfg.json" -X POST http::/localhost:8080/config
```
#### Shutdown server
```bash
curl -d "" -X POST http::/localhost:8080/shutdown
```
## Config file
#### All config files must be in /cfg/ directory
#### Example of cfg file:
```json
{
    "ProcessingTime": 5000,
    "OperatorsCount": 3,
    "QueueSize": 10,
    "Rmin": 1000,
    "Rmax": 5000
}
```
## Where journal files creating(relative from executable)
#### CDR: ../cdr/
#### Logs: ../log/
#### Statistics: ../stats/
