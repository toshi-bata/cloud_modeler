# Cloud modeler demo
Cloud modeler demo is able to create 3D models from scratch using Parasolid which is running in the server side. It is also can import 3D CAD data using Exchange which is also runnning in the server side. <br>
This demo is introduced in our Innovation Lab<br>
https://labs.techsoft3d.com/project/cloud-modeler

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
&emsp;+ cloud_modeler<br>
&emsp;&emsp;+ css<br>
&emsp;&emsp;+ js<br>
&emsp;&emsp;+ PsServer<br>
&emsp;&emsp;+ Release<br>
&emsp;&emsp;&emsp;+ scheme (Locate schema folder from `Parasolid SDK\base`)<br>
&emsp;&emsp;&emsp;+ PsServer.exe<br>
&emsp;&emsp;&emsp;+ Parasolid dll files (locate all files in `Parasolid SDK\base\bin`)<br>
&emsp;&emsp;&emsp;+ Exchange dll files (locate all files in `Exchange SDK\bin\win64_v142`)<br>
&emsp;&emsp;+ src (copy src folder from `Communicator SDK\web_viewer`)<br>
&emsp;&emsp;+ cloud_modeler.html<br>
&emsp;&emsp;+ HttpServer.js<br>
&emsp;&emsp;+ package.json<br>

### Environment valiables
Register the following enviroment valiables<br>
    Name: `PS_SERVER_WORKING_DIR`   Value: C:\temp\PsServer\ (Create the folder if it doen't exist)

### PsServer
PsServer is a server side console application which is using HOOPS Exchange, Parasolid. <br>
\* As of now, it will works on Windows only<br>
If you want to build PsServer, <br>
1. Edit `cloud_modeler\PsServer\HC_VS2019.bat` according to your environment
2. Launch the `PsServer.sln` solution with VS2019 using the above bat file
3. Build the console application project

## Start cloud_modeler
1. Launch HTTP server using a command prompt<br>
    `cd …/cloud_modeler`<br>
    `npm install`<br>
    `npm start`<br>
2. Start PsServer.exe 8889 (giving a port number in command line argument)
3. Open (using Chrome)<br>
    http://localhost:8000/cloud_modeler.html?viewer=scs&instance=_empty.scs&port=8889

