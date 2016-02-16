# Configuring NTP to use the PPS Signal

Once the kernel is recompiled (and rebooted), the module is installed, and the GPS is wired in, you’ll need to update `/etc/ntp.conf` to use the new reference clock. Add the following lines to /etc/ntp.conf and restart the server. *You will also need to select a preferred `ntp` server from your list of servers*.

```
server 127.127.22.0
fudge 127.127.22.0 flag3 1 
```

At this point, if all is working, `PPS` will show up in the NTP peer list.

```
% ntpq
ntpq> peers
     remote           refid      st t when poll reach   delay   offset  jitter
==============================================================================
…
 PPS(0)          .PPS.            0 l    -   64    0    0.000    0.000   0.000
…
```

It may take a few minutes after the GPS gets a fix for NTP to begin accepting the `PPS` signal. Once the GPS signal has settled in, `ntptime` will show a status that includes `(PLL,PPSFREQ,PPSTIME,NANO)`. 

In addition, the `ntpq -pcrv` command will show the happy status of

```
%ntpq -pcrv
...
     stratum=1 
...
```

If these commands do not show said values, there's a wealth of information on the Web about configuring NTP. 

# Configuring the PI to Serve Time

Read the documents for `ntp.conf` and add the appropriate `restrict` directives to your system.

