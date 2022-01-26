# AFLNet Walkthrough

## AFLNet Installation

1. Download the AFLNet's Dockerfile used in an official [demo](https://github.com/aflnet/aflnet/blob/master/tutorials/lightftp/Dockerfile)
2. Modify the Dockerfile by completing the following steps (the final Dockerfile could be found [here](Dockerfile))
    1. Update the operating system (with one supported by AFLNet)
    2. Modify the dependencies, including LVMM 6.0 and `psmisc`
    3. Adjust the instructions to not involve the tested software, namely LightFTP
3. Build the image: `sudo docker build . --tag aflnet`
4. Create a container based on the image: `sudo docker run --detach --interactive aflnet --name aflnet-instace /bin/bash`
5. Run the container `sudo docker exec --interactive --tty aflnet-instance /bin/bash`

## Fuzzing an Open Source Project

1. Find an open source project. In this case, a popular embeddable C web server, CivetWeb, was chosen.
2. Clone the repository: `git clone https://github.com/civetweb/civetweb`
3. Build the project with instrumentation: `CC=afl-clang CXX=afl-clang++ make WITH_ALL=1`
4. Install the server: `sudo make install`
5. Enable logging: `echo -e "\naccess_log_file /home/ubuntu/log.txt\n" >> /usr/local/etc/civetweb.conf`
6. Create a script with the content of [`run_server.sh`](run_server.sh) that ensure the server's restart

```
chmod +x run_server.sh
./run_server.sh
```

1. Record some valid requests from the host operating system, with Wireshark. Mock ones are offered in the [`captures`](captures) folder.
    1. Obtain the IP address of the container, namely `CONTAINER_IP`: `docker
    2. Set a filter for `http and tcp.dst == CONTAINER_IP`
    3. Follow the HTTP streams
    4. Save only the requests (from host to container) in binary format
    5. Move the captured traffic into the container
2. Fuzz the server: `afl-fuzz -d -i captures -o results -x civetweb/fuzztest/http1.dict -N tcp://172.17.0.3/8080 -P HTTP -D 10000 -q 3 -s 3 -E -R /usr/local/bin/civetweb`
3.  Wait some minutes

```
                      american fuzzy lop 2.56b (civetweb)

┌─ process timing ─────────────────────────────────────┬─ overall results ─────┐
│        run time : 0 days, 0 hrs, 13 min, 30 sec      │  cycles done : 185    │
│   last new path : 0 days, 0 hrs, 11 min, 42 sec      │  total paths : 13     │
│ last uniq crash : none seen yet                      │ uniq crashes : 0      │
│  last uniq hang : none seen yet                      │   uniq hangs : 0      │
├─ cycle progress ────────────────────┬─ map coverage ─┴───────────────────────┤
│  now processing : 2* (15.38%)       │    map density : 1.18% / 1.18%         │
│ paths timed out : 0 (0.00%)         │ count coverage : 1.04 bits/tuple       │
├─ stage progress ────────────────────┼─ findings in depth ────────────────────┤
│  now trying : splice 5              │ favored paths : 1 (7.69%)              │
│ stage execs : 15/16 (93.75%)        │  new edges on : 1 (7.69%)              │
│ total execs : 213k                  │ total crashes : 0 (0 unique)           │
│  exec speed : 266.2/sec             │  total tmouts : 0 (0 unique)           │
├─ fuzzing strategy yields ───────────┴───────────────┬─ path geometry ────────┤
│   bit flips : n/a, n/a, n/a                         │    levels : 4          │
│  byte flips : n/a, n/a, n/a                         │   pending : 0          │
│ arithmetics : n/a, n/a, n/a                         │  pend fav : 4.29G      │
│  known ints : n/a, n/a, n/a                         │ own finds : 11         │
│  dictionary : n/a, n/a, n/a                         │  imported : n/a        │
│       havoc : 8/84.2k, 3/129k                       │ stability : 100.00%    │
│        trim : n/a, n/a                              ├────────────────────────┘
└─────────────────────────────────────────────────────┘          [cpu000: 42%]
```

10. Inspect the logs to see the mutations in the network protocol (in this case, filenames and user agents)

```
[...]
172.17.0.3 - - [24/Jan/2022:12:01:12 +0000] "GET /civetweb_64x64.png HTTP/1.1" 304 183 - Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:96.0) Gecko/20100101 Firefhx//html,application/xhtml+xml,application/xml;q=0.9,
image/avif,image/webp,*/*;q=0.8
[...]
172.17.0.3 - - [24/Jan/2022:12:01:18 +0000] "GET /civetweb_64xX4.png HTTP/1.1" 404 255 -
[...]
```

11. Check the results of the fuzzing session: `cat results/fuzzer_stats`

```
start_time        : 1643025982
last_update       : 1643026079
fuzzer_pid        : 17465
cycles_done       : 20
execs_done        : 11402
execs_per_sec     : 121.95
paths_total       : 2
paths_favored     : 1
paths_found       : 0
paths_imported    : 0
max_depth         : 1
cur_path          : 0
pending_favs      : 0
pending_total     : 0
variable_paths    : 0
stability         : 100.00%
bitmap_cvg        : 0.53%
unique_crashes    : 0
unique_hangs      : 0
last_path         : 0
last_crash        : 0
last_hang         : 0
execs_since_crash : 11402
exec_timeout      : 60
afl_banner        : civetweb
afl_version       : 2.56b
target_mode       : default
command_line      : afl-fuzz -d -i captures -o results -x civetweb/fuzztest/http1.dict -N tcp://172.17.0.3/8080 -P HTTP -D 1000 -q 3 -s 3 -E -R /usr/local/bin/civetweb
slowest_exec_ms   : 33
peak_rss_mb       : 2
```

## Vulnerability Insertion. Server Crashing

1. Modify the source code of `civetweb.c` to insert a vulnerable function definition

```
static unsigned char *
get_user_agent(const struct mg_connection *conn)
{
    char *user_agent, *lowercased_user_agent;

    // Allocate some space
    lowercased_user_agent = (char *)mg_malloc(80 * sizeof(char));

    // Get the initial user agent value
    user_agent = header_val(conn, "User-Agent");

    // Process the user agent
    for (size_t i = 0; i < strlen(user_agent); ++i) {
        lowercased_user_agent[i] = tolower(user_agent[i]);
    }

    return lowercased_user_agent;
}
```

2. Call the vulnerable function (on the line `15986` of the initial file)

```
/* Some extra buggy processing */
char *processed_user_agent =  process_user_agent(user_agent);
```

3. Build and run the server and the fuzzer as described in the steps above
4. Wait for the crash of the server (just a few seconds)

```
Loading config file /usr/local/etc/civetweb.conf
CivetWeb V1.16 started on port(s) 8080 with web root [/usr/local/share/doc/civetweb]
[..]
free(): invalid next size (normal)
./run_forever.sh: line 8: 251702 Aborted                 sudo /usr/local/bin/civetweb
[..]
```

```
                      american fuzzy lop 2.56b (civetweb)

┌─ process timing ─────────────────────────────────────┬─ overall results ─────┐
│        run time : 0 days, 0 hrs, 0 min, 8 sec        │  cycles done : 0      │
│   last new path : 0 days, 0 hrs, 0 min, 7 sec        │  total paths : 4      │
│ last uniq crash : none seen yet                      │ uniq crashes : 0      │
│  last uniq hang : 0 days, 0 hrs, 0 min, 0 sec        │   uniq hangs : 3      │
├─ cycle progress ────────────────────┬─ map coverage ─┴───────────────────────┤
│  now processing : 0 (0.00%)         │    map density : 1.18% / 1.18%         │
│ paths timed out : 0 (0.00%)         │ count coverage : 1.01 bits/tuple       │
├─ stage progress ────────────────────┼─ findings in depth ────────────────────┤
│  now trying : splice 9              │ favored paths : 1 (25.00%)             │
│ stage execs : 5/16 (31.25%)         │  new edges on : 1 (25.00%)             │
│ total execs : 678                   │ total crashes : 0 (0 unique)           │
│  exec speed : 1.21/sec (zzzz...)    │  total tmouts : 3 (3 unique)           │
├─ fuzzing strategy yields ───────────┴───────────────┬─ path geometry ────────┤
│   bit flips : n/a, n/a, n/a                         │    levels : 2          │
│  byte flips : n/a, n/a, n/a                         │   pending : 4          │
│ arithmetics : n/a, n/a, n/a                         │  pend fav : 1          │
│  known ints : n/a, n/a, n/a                         │ own finds : 2          │
│  dictionary : n/a, n/a, n/a                         │  imported : n/a        │
│       havoc : 2/512, 0/128                          │ stability : 100.00%    │
│        trim : n/a, n/a                              ├────────────────────────┘
└─────────────────────────────────────────────────────┘          [cpu000: 55%]
```

5. As the server is not multithreaded, it does not response on crash, so the crashes are hands which can be inspected as follows:

```
ls results/replayable-hangs/
id:000000,src:000000,op:havoc,rep:8
```

6. Check the payload which triggers the vulnerability

```
tail -1 log.txt 
172.17.0.3 - - [24/Jan/2022:13:55:28 +0000] "GET /civetweb_64x64.png HTTP/1.1" 304 183 - Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:96.0) Gecko/20100101 Firefox/96.0��Accept: image/avif,im,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8
```