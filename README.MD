# Cloud modeler demo
Cloud modeler demo is able to create a 3D model from scratch using Parasolid which is running in a server side. It is also can an import 3D CAD data using Exchange which is also runnning in a server side. 

## Dependencies
### SDKs in use (version)
- HOOPS Communicator (2024)
- HOOPS Exchange (2024)
- Parasolid (34.1)

### Third paty module
- libmicrohttpd (1.0.0-w32-bin)<br>
  https://ftp.gnu.org/gnu/libmicrohttpd/

## Setup
### Demo folders
&emsp;+ cloud_modeler
&emsp;&emsp;+ PsServer
&emsp;&emsp;+ cloud_modeler.html
&emsp;&emsp;+ Release
&emsp;&emsp;&emsp;+ scheme (Parasolid scheme folder)
&emsp;&emsp;&emsp;+ PsServer.exe
&emsp;&emsp;&emsp;+ Parasolid dll files
&emsp;&emsp;&emsp;+ Exchange dll files
&emsp;&emsp;+ src (copy src folder from <Communicator SDK>\web_viewer)

### Environment valiables
Register the following enviroment valiables<br>
    Name: PS_SERVER_WORKING_DIR   Value: C:\temp\PsServer\ (Create the folder if it doen't exist)

### PsServer
PsServer is a server side console application which is using HOOPS Exchange, Parasolid. 
/* As of now, it will works on Windows only
1. Locate all files in <Parasolid SDK>\base\bin of Parasolid to the ..\Release folder
2. Locate schema folder in <Parasolid SDK>\base to the ..\Release folder
3. Locate all files in <Exchange SDK>\bin\win64_v140 to the ..\Release folder

If you want to rebuild PsServer application
1. Edit cloud_modeler\PsServer\HC_VS2019.bat according to your environment
2. Launch the solution with VS2019 using the above bat file
3. Build the console application project

## Start cloud_engineering for release using reverse proxy
1. Launch a HTTP server for relase
2. Start ProcessServer.exe 8888 (giving a port number in command line argument)
3. Open (using Chrome)<br>
    http://cloud_modeler/cloud_modeler.html?viewer=scs&instance=_empty.scs&proxy=on&port=8888
        
## Start cloud_modeler for debug (without using ProcessServer)
1. Launch HTTP server using a command prompt<br>
    > cd …/cloud_modeler
    > npm install
    > npm start
2. Start PsServer.exe 8889 (giving a port number in command line argument)
3. Open (using Chrome)<br>
    http://localhost:8000/cloud_modeler.html?viewer=scs&instance=_empty.scs&port=8889

