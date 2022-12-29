# About Phone Number Reverse Proxy

This project enables you to use a regular mobile phone number - as a full reverse proxy. All SMS messages are forwarded, calls are redirected. Proper health checking, allowing you to not worry at night whether or not it actually works - lol.

Why?

Mobile phone numbers leak a lot, if not in Twitter databases - via all the AliExpress packages that paste it on there. Does every Takeaway restaurant really need your personal phone number? For those that want to limit the risk of having to fully replace their primary phone number (like myself), this is a great solution.

VOIP is not an option. I have extensive experience with VOIP, and having a receiving (normal) number for calls and SMS messages - is not possible. You are able to create two seperate VOIP phone numbers to receive both respectivly, but even then it has limitations like minimal support for SMS messages from outside of your host country. The numbers itself are also not what you're used to.

# Get started

You'll need around 22EUR for the hardware, excluding SIM card costs. I personally used the TTGO SIM800 + ESP32 - so just buy that if you don't want headaches. It doesn't matter which SIM800 type of board you choose.

Options (disclosure; reflinks);
- https://opencircuit.nl/product/lilygo-ttgo-t-call-pmu-esp32-wireless-sim?cl=DC9ZYNYEF9&cid=SMS%20Proxy
- https://opencircuit.nl/product/lilygo-ttgo-t-call-esp32-with-sim800c?cl=DC9ZYNYEF9&cid=SMS%20Proxy

Or if you care less about PCB quality / shipping time -> just buy from AliExpress.

You'll need the following software;
- Arduino IDE
    - https://docs.arduino.cc/software/ide-v2
    - https://github.com/vshymanskyy/TinyGSM (library zip install)
        - https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries

In Arduino IDE, install the esp32 board manager pack. As can be found here; 
- https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/

Board type to use (don't mess with the config); `ESP32 Wrover Module`

Adjust the global parameters;
```
const String PHONE_NUMBER = "+31612345678"; //always with country code like this +31612345678
const char* SSID          = "";
const char* PASSWORD      = "";
String HEARBEAT_FULL_URL  = ""; //Free and good service I personally use for this (hearbeat); https://cronitor.io/
unsigned long HEARTBEAT_INTERVAL     = 600000; // 600000 milliseconds - 10 min
unsigned long HEARTBEAT_LAST_TIME   = 0; //// milliseconds
```
Make sure you SIM has no PIN lock. There is code available in the project which you can comment out if you really want it (not tested though).
```
// Unlock your SIM card with a PIN
//modem.simUnlock("1234");
```

Do NOT flash it yet, without reading about the SIM card options and call redirection options.

## SIM card options (NL)

This is where things get interesting. SMS works fine regardless of provider, but call redirection not. In general, any prepaid card does not support call redirection configuration via the SIM card. Hence it is disabled by default in the codebase;

```
// SerialMonitor.println("Configuring SIM800 to redirect phone calls... (method 1)");
// SerialSIM800.println("AT+CCFC=0,3,\"" + PHONE_NUMBER + "\",145");

// SerialMonitor.println("Configuring SIM800 to redirect phone calls... (method 2)"); // was not able to test, use modem as it's nicer
// SerialSIM800.println("AT+CUSD=1,\"**21*" + PHONE_NUMBER + "\"#"); 
// modem.sendUSSD("**21*" + PHONE_NUMBER + "\"#")
```
On the KPN forum there is some reasoning to be found why that is (even though I think it's bs); https://forum.kpn.com/prepaid-16/doorschakelen-met-een-prepaid-478998

There are exceptions though. I tested Lyca Mobile (not supported) and Vodafone (partial support). KPN seems to suggest in docs to also support it for prepaid; but always call customer service to make sure before buying!

If you want to use Vodafone prepaid;
- Get the SIM (mine was 5 EUR, with 5 EUR balance)
- Activate in a smartphone - not your SIM800
- Call customer service, ask to not send you SMS messages each time your balance lowers 
    - If you don't do this, the SMS forwarder will loop and spend all your balance - haha.
- Ask customer service for an online MyVodafone account
    - They only require an e-mail address, no additional details
- Configure call redirection via the MyVodafone portal

Most tariffs are around 15 cent per SMS/Minute, so threshold is around 40 units before a subscription becomes interesting. But I honestly don't see that happening for me in any scenario.

In general, if you want to do this automatically - you will be required to buy a SIM only subscription (Youfone - 6EUR p/m) - as docs do suggest they allow it there.

---

Now flash away.

You'll always receive an SMS on boot, so you'll know the number + know that at least SMS works. Always test if everything works, but goes without saying...

## Limitations

- For those using prepaid cards, the health checking does not check for your balance. It can be assumed you'll receive an SMS from the provider in advance - but please verify this before creating issues.
- As call redirection is done on the provider network level, and not via the SIM card itself - you won't know when called on your primary phone whether it's a forwarded call or not. As far as I know, there is no way around this. I don't mind, but keep this in mind when calling back or when the number they cite is not what you expected...
- This project is not a fully fledged proxy, meaning - you can't call or SMS back to the sender with the proxy number. I have no ambition nor need for such functionality. Full proxy functionality for SMS would be possible, calling not. I would not recommend it though. If you want a proxy -> use VOIP (way cheaper and easier). You can very easily spoof SMS messages using API calls with all major companies (like Messagebird or Twilio). You can also spoof your calls interactivly as I blogged about earlier this year; https://cozybear.dev/posts/spoofing-any-call-interactively/
