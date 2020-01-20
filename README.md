# SerialPort Over Socket
Redirect serial port to socket

## Compilation
```
cmake .
make
```

## Parameters
```
-c,--config             configure file path
-g,--generate           generate default configure file,
                        if the configure file is not exists
-p,--port               port device
-b,--bandrate           baundrate
-d,--databits           databits
-s,--stopbit            stopbit
-r,--parity             parity
-l,--log                start log and log filepath
-h,--help               print out this help text

```

## RoadMap

### Server
[] password support  
[x] snippet support and manager

### Client
[x] control+Q enter admin interface  
[] log to file  
[] change port configuration runtime

