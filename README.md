Garage Monitor for Feather Huzzah and Twilio
========

* Garage Monitor for Feather Huzzah and Twilio

* It now relies on a local server to keep the Twilio SHA1 fingerprint up to date. 
* So it makes a HTTP request to a local server of the text file with the fingerprint and 
* does so once per hour. The local server updates the fingerprint periodically using
* openssl s_client -connect api.twilio.com:443 2>/dev/null | openssl x509  -noout -fingerprint | cut -f2 -d'=' > /volume1/web/twilio_fp.txt

* Blue LED is on if door is open

* Navigate to http://_ip_address (i.e. the IP address of the Huzzah) to get the garage's current status
