# fileTI
FTP client :) <br>
  !!!program still in development.!!!<br>

<p align="center"><img src="https://github.com/gomnarija/fileTI/blob/master/ss/example_1.png" alt="example" width=500 height=360 /></p>

## Installation

```sh
git clone https://github.com/gomnarija/fileTI.git
cd fileTI
make install
```


## Usage
Program consists of two panes, local and server, and an additional log pane.<br>
All commands are applied to currently active pane, except pane specific commands (rtr,snd,wd).<br>
Press '/' to enter command mode.<br>
Command format:<br>
```
    /<command> <argument> <argument>
```
Distance between each word must be one space character.
All commands are in lowercase.
| Command | Description | Arguments | Key |
| ------ | ------ | ------ | ------ | 
| connect | Control connection with FTP server | IPv4,port |
| login | Login to the server | user,password |
| rtr| Retrieves file/dir from the server | src_name,dst_name |  r
| snd| Sends file to the server | src_name,dst_name | s
| ls | Lists current dir  | 
| rm/rmdir | Removes file/dir  | name
| wd | Return to local working dir  | 
|   | Selected up. |   |KEY_UP
|   | Selected down. |   |KEY_DOWN
|   | Switch betweem local/server panes.  |   |KEY_SPACE
|   | Enter/exit log pane.  |   |z
|   | Enter selected dir.  |   |e
|   | Retrieve file/dir, dst_name=src_name.  |   |r
|   | Send file, dst_name=src_name.  |   |s
|   | Remove.          |   |d
|   | Command mode.          |   |/
|   | Quit.          |   |q


## Some notes
!!!program still in development.!!!<br>
ftp server must support MLSD command.<br>
passwords are (currently?) not hidden, so be carefull about that :).<br>
## License

GPL-3.0
