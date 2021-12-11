# Chatting Room
A simple chatting room implementation in socket.You can use telnet to connect to the server.
***

## Usage
Just connect to the server in `telent` command from terminal,then apply commands below.

## Commands
`cr register <username> <password>`  
register new user

`cr login <username> <password>`  
login to the server.  

`cr logout`  
logout from the server.

`cr list`  
list all available chatting room.

`cr add <room_name>`  
create chatting room named room_name.

`cr delete <room_name>`  
delete chatting room named room_name.

`cr join <room_name>`  
join chatting room named room_name.

`cr exit`  
exit from current joined chatting room.

`<enter something that not starts with 'cr'>`  
send messasge to all users in chatting room that you current joined.