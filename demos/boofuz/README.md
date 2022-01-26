# boofuzz Walkthrough

1. Make sure that you have Python 3, `make` and `gcc` packets installed on your machine

```bash
sudo apt update
sudo apt install gcc make python3
python3 --version 
```

2. Install `python3-pip` by running

```bash
sudo apt-get update
sudo apt-get -y install python3-pip
pip3 --version
```

3. The last command `pip3 --version` should not generate any error

4. Next we are going to install [boofuzz](https://github.com/jtpereyda/boofuzz) by running. The documentation is available [here](https://boofuzz.readthedocs.io/).

```bash
pip3 install boofuzz
echo $? # It sould say 0 (this is the exit code of the last command)
```

5. Now we've got our basic setup. The next step is to compile and run the server with the following commands.

```bash 
make # This sould compile the server.c into server
./restart_server.sh
```

6. Open a new terminal and run the fuzzer with the following command

```bash
python3 fuzzer.py
```

7. A web interface is provided by the boofuzz and it can be accessed at [http://localhost:26000](http://localhost:26000) there you can see some statistics and test cases that generated a crash or some sort of error at the server level.