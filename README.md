This program makes use of a proxy server to dynamically block and unblock webpages based on certain keywords of the URL. 

Making use of 2 pthreads, the filterThread allows the dynamic blocking and unblocking of the webpages and a webThread that redirects clients/users to webpages (if a certain webpage is blocked, the client is redirected to an error page). 

I also made use of socket programming to connect to various webservers and the "filtering" server.


**To run this program, first configure the HTTP Proxy and Port number to match the Proxy Server address. Connect to the "filter" server to BLOCK and UNBLOCK webpages based on URL keywords. (Filter server has the same IP address, but the port number would be different). 
(Use telnet to connect to the filter server). 
Run the Proxy.c file.
After connecting to the filter server, requests from the client will be displayed on the server terminal and to block a webpage, simply type, BLOCK <Keyword>. To unblcok a webpage, type, UNBLOCK <KeyWord>.**