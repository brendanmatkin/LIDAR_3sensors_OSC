void initNetwork() {
  Serial.printf("Connecting to network..\n");
  switch (DHCP) {
    case IP_DNS_GATE_SUB:
      //Ethernet.begin(mac, myIP, dns, gateway);
      //Serial.print("IP: ");
      //Serial.println(Ethernet.localIP());
      break;
    case IP:
      Ethernet.begin(mac, myIP);
      Serial.printf("Static IP:\t\t");
      Serial.println(Ethernet.localIP());
      break;
    case AUTO:
      if (Ethernet.begin(mac)) {
        Serial.print("Assigned IP:\t\t");
        Serial.println(Ethernet.localIP());
      }
      else Serial.println("Failed to connect to DHCP");
      break;
    default:
      Serial.println("network not configured");
      break;
  }
  //Ethernet.begin(mac, IPAddress(10,0,1,100));

  Serial.print("Destination Port:\t");
  if (udp.begin(destPort)) Serial.println(destPort);
  else Serial.printf("%i not available\n", destPort);

  Serial.printf("Network Setup Complete\n");
}
