# Smart Doorbell

Slightly misleading name, but this was an ordianry wireless doorbell with an added NodeMCU Microprocessor to enable additional features.

### The Features of this sketch are:

- To control the doorbell's chime or put it in silent mode. *Note I added a transister to the doorbell to allow for this*
- MQTT protocol
- Publishing when doorbell button is pushed to allow the central server to send a push notification that someone is at the door and soon a CCTV snap image of who is at the door
- Finally this microprocessor has a DHT22 sensor reporting the temperature and Humidity of the kitchen (That is where the doorbell device is mounted in my case.)
