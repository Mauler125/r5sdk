tw.py is a python script to act as a test server to simulate the conditions below.  The syntax is python tw.py --ip <ip of your box> --handler <which test to run>.  
By default the server will listen on port 9000.

1.     Server responds normally to request.
python tw.py --ip 10.30.80.175 --handler normal

2.     Server is not responding to requests.
python tw.py --ip 10.30.80.175 --handler noresponse

The server will not respond with a proper HTTP response.

3.     Server is responding slowly to requests.
python tw.py --ip 10.30.80.175 --handler slow

Server will respond in 30s

4.     Server is responding with a certain HTTP error code.
python tw.py --ip 10.30.80.175 --handler error --error 403

5.     Server is not listening on a port at all.
Don't start the server.

